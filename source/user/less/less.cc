/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <esc/proto/vterm.h>
#include <esc/stream/fstream.h>
#include <esc/stream/std.h>
#include <esc/env.h>
#include <sys/common.h>
#include <sys/esccodes.h>
#include <sys/keycodes.h>
#include <sys/messages.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "linecontainer.h"

using namespace std;

static void resetVterm(void);
static void readLines(size_t end);
static void append(string& s,int& len,char c);
static void scrollDown(long lines);
static void refreshScreen(void);
static void printStatus(const char *totalStr);

static const int TAB_WIDTH = 4;

static esc::FStream *vt;
static FILE *in;
static const char *filename;
static bool seenEOF;
static LineContainer *lines;
static size_t startLine = 0;
static esc::VTerm vterm(esc::env::get("TERM").c_str());
static esc::Screen::Mode mode;
static string emptyLine;

static void usage(const char* name) {
	esc::serr << "Usage: " << name << " [<file>]" << '\n';
	esc::serr << "    navigation:" << '\n';
	esc::serr << "        up/down         : one line up/down" << '\n';
	esc::serr << "        pageup/pagedown : one page up/down" << '\n';
	esc::serr << "        home/end        : to the very beginning or end" << '\n';
	esc::serr << "        q               : quit" << '\n';
	esc::serr << "        s               : stop reading (e.g. when walking to EOF)" << '\n';
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	char c;
	bool run = true;

	if(argc > 2 || getopt_ishelp(argc,argv))
		usage(argv[0]);

	// open file or use stdin
	if(argc > 1) {
		filename = argv[1];
		in = fopen(filename,"r");
		if(in == nullptr)
			error("Unable to open '%s'",filename);
	}
	else if(isatty(STDIN_FILENO))
		error("Using a vterm as STDIN and have got no filename");
	else {
		filename = "STDIN";
		in = stdin;
	}

	// get vterm size
	mode = vterm.getMode();
	// one line for the status
	mode.rows--;
	lines = new LineContainer(mode.cols);

	// backup screen
	vterm.backup();

	// create empty line
	emptyLine.assign(mode.cols,' ');

	/* open the "real" stdin, because stdin maybe redirected to something else */
	vt = new esc::FStream(esc::env::get("TERM").c_str(),"r");

	// read the first lines into it
	seenEOF = false;
	readLines(mode.rows);

	// stop readline and navigation
	vterm.setFlag(esc::VTerm::FL_READLINE,false);
	vterm.setFlag(esc::VTerm::FL_NAVI,false);

	refreshScreen();

	// read from vterm
	while(run && (c = vt->get()) != EOF) {
		if(c == '\033') {
			int n1,n2,n3;
			int cmd = vt->getesc(n1,n2,n3);
			if(cmd != ESCC_KEYCODE || (n3 & STATE_BREAK))
				continue;
			switch(n2) {
				case VK_Q:
					run = false;
					break;
				case VK_HOME:
					// scrollDown(0) means scroll to end..
					if(startLine > 0)
						scrollDown(-startLine);
					break;
				case VK_END:
					scrollDown(0);
					break;
				case VK_UP:
					scrollDown(-1);
					break;
				case VK_DOWN:
					scrollDown(1);
					break;
				case VK_PGUP:
					scrollDown(-(long)mode.rows);
					break;
				case VK_PGDOWN:
					scrollDown(mode.rows);
					break;
			}
		}
	}

	if(in != stdin)
		fclose(in);
	resetVterm();

	return EXIT_SUCCESS;
}

static void resetVterm(void) {
	vterm.setFlag(esc::VTerm::FL_READLINE,true);
	vterm.setFlag(esc::VTerm::FL_NAVI,true);
	vterm.restore();
}

static void scrollDown(long l) {
	size_t oldStart = startLine;
	if(l < 0) {
		if((long)startLine + l >= 0)
			startLine += l;
		else
			startLine = 0;
	}
	else if(lines->size() >= mode.rows)
		startLine += l;

	if(l == 0 || startLine > lines->size() - mode.rows) {
		readLines(l == 0 ? 0 : startLine + mode.rows);
		if(lines->size() < mode.rows)
			startLine = 0;
		else if(l == 0 || startLine > lines->size() - mode.rows)
			startLine = lines->size() - mode.rows;
	}

	if(oldStart != startLine)
		refreshScreen();
}

static void refreshScreen(void) {
	size_t j = 0;
	LineContainer::size_type end = startLine + min(lines->size(),(size_t)mode.rows);
	// walk to the top of the screen
	esc::sout << "\033[mh]";
	for(LineContainer::size_type i = startLine; i < end; ++i, ++j) {
		esc::sout << lines->get(i);
		if(j < mode.rows - 1)
			esc::sout << '\n';
	}
	for(; j < mode.rows; ++j) {
		esc::sout << emptyLine;
		if(j < mode.rows - 1)
			esc::sout << '\n';
	}
	printStatus(seenEOF ? nullptr : "?");
	esc::sout.flush();
}

static void printStatus(const char *totalStr) {
	esc::OStringStream lineStr;
	size_t end = min(lines->size(),(size_t)mode.rows);
	lineStr << "Lines " << (startLine + 1) << "-" << (startLine + end) << " / ";
	if(!totalStr)
		lineStr << lines->size();
	else
		lineStr << totalStr;
	esc::sout << "\033[co;0;7]";
	esc::sout << lineStr.str() << esc::fmt(filename,mode.cols - lineStr.str().length());
	esc::sout << "\033[co]";
}

static void readLines(size_t end) {
	int lineCount = 0;
	int lineLen = mode.cols;
	string line;
	static const char *states[] = {"|","/","-","\\","|","/","-"};
	int state = 0;
	if(seenEOF)
		return;

	// read
	char c = 0;
	while(c != EOF && (end == 0 || lines->size() < end)) {
		int len = 0;
		line.clear();
		while((c = fgetc(in)) != EOF && c != '\n') {
			if(len < lineLen)
				append(line,len,c);
		}
		lines->append(line.c_str());

		// check whether the user has pressed a key
		if(lineCount++ % 100 == 0) {
			char vtc;
			fcntl(vt->fd(),F_SETFL,O_NONBLOCK);
			while((vtc = vt->get()) != EOF) {
				if(vtc == '\033') {
					int n1,n2,n3;
					int cmd = vt->getesc(n1,n2,n3);
					if(cmd != ESCC_KEYCODE || (n3 & STATE_BREAK))
						continue;
					if(n2 == VK_S) {
						vt->clear();
						fcntl(vt->fd(),F_SETFL,0);
						return;
					}
				}
			}
			vt->clear();
			fcntl(vt->fd(),F_SETFL,0);

			esc::sout << '\r';
			printStatus(states[state]);
			state = (state + 1) % ARRAY_SIZE(states);
		}
	}
	seenEOF = c == EOF;
}

static void append(string& s,int& len,char c) {
	static int inEsc = 0;
	switch(c) {
		case '\t':
			for(int i = TAB_WIDTH - s.size() % TAB_WIDTH; i > 0; i--) {
				s += ' ';
				len++;
			}
			break;
		case '\b':
		case '\a':
		case '\r':
			// ignore
			break;

		default:
			// TODO actually, it would be cool to support escape-codes here, but the linecontainer
			// can't cope with "invisible" characters that lead to longer lines. so for now, just
			// remove the escape-codes.
			if(inEsc) {
				if(c == ']')
					inEsc = 0;
			}
			else if(c == '\033')
				inEsc = 1;
			else {
				s += c;
				len++;
			}
			break;
	}
}
