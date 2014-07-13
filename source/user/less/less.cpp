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

#include <sys/common.h>
#include <sys/keycodes.h>
#include <sys/messages.h>
#include <sys/esccodes.h>
#include <dirent.h>
#include <esc/proto/vterm.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmdargs.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <env.h>
#include "linecontainer.h"

#define TAB_WIDTH			4

using namespace std;

static void resetVterm(void);
static void readLines(size_t end);
static void append(string& s,int& len,char c);
static void scrollDown(long lines);
static void refreshScreen(void);
static void printStatus(const char *totalStr);

/* TODO when building for eco32, there seems to be an alignment problem. if we use a
 * non-word-sized entity, _cin will be put to a non-word-aligned address. but the generated code
 * will access a word there. thus, this can't work. it seems to be a bug in the eco32-specific part
 * of ld? */

static ifstream vt;
static FILE *in;
static string filename;
static bool seenEOF;
static LineContainer *lines;
static size_t startLine = 0;
static esc::VTerm vterm(std::env::get("TERM").c_str());
static esc::Screen::Mode mode;
static string emptyLine;

static void usage(const char* name) {
	cerr << "Usage: " << name << " [<file>]" << '\n';
	cerr << "    navigation:" << '\n';
	cerr << "        up/down         : one line up/down" << '\n';
	cerr << "        pageup/pagedown : one page up/down" << '\n';
	cerr << "        home/end        : to the very beginning or end" << '\n';
	cerr << "        q               : quit" << '\n';
	cerr << "        s               : stop reading (e.g. when walking to EOF)" << '\n';
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	char c;
	bool run = true;

	// parse args
	cmdargs args(argc,argv,cmdargs::MAX1_FREE);
	try {
		args.parse("");
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		cerr << "Invalid arguments: " << e.what() << '\n';
		usage(argv[0]);
	}

	// open file or use stdin
	if(args.get_free().size() > 0) {
		filename = *args.get_free().at(0);
		in = fopen(filename.c_str(),"r");
		if(in == nullptr)
			error("Unable to open '%s'",filename.c_str());
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
	vt.open(env::get("TERM").c_str());

	// read the first lines into it
	seenEOF = false;
	readLines(mode.rows);

	// stop readline and navigation
	vterm.setFlag(esc::VTerm::FL_READLINE,false);
	vterm.setFlag(esc::VTerm::FL_NAVI,false);

	refreshScreen();

	// read from vterm
	while(run && (c = vt.get()) != EOF) {
		if(c == '\033') {
			istream::esc_type n1,n2,n3;
			istream::esc_type cmd = vt.getesc(n1,n2,n3);
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
	cout << "\033[mh]";
	for(LineContainer::size_type i = startLine; i < end; ++i, ++j) {
		cout << lines->get(i);
		if(j < mode.rows - 1)
			cout << '\n';
	}
	for(; j < mode.rows; ++j) {
		cout << emptyLine;
		if(j < mode.rows - 1)
			cout << '\n';
	}
	printStatus(seenEOF ? nullptr : "?");
	cout.flush();
}

static void printStatus(const char *totalStr) {
	ostringstream lineStr;
	size_t end = min(lines->size(),(size_t)mode.rows);
	lineStr << "Lines " << (startLine + 1) << "-" << (startLine + end) << " / ";
	if(!totalStr)
		lineStr << lines->size();
	else
		lineStr << totalStr;
	cout << "\033[co;0;7]";
	cout << lineStr.str() << setw(mode.cols - lineStr.str().length()) << right << filename;
	cout << left << "\033[co]";
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
			fcntl(vt.filedesc(),F_SETFL,O_NONBLOCK);
			while((vtc = vt.get()) != EOF) {
				if(vtc == '\033') {
					istream::esc_type n1,n2,n3;
					istream::esc_type cmd = vt.getesc(n1,n2,n3);
					if(cmd != ESCC_KEYCODE || (n3 & STATE_BREAK))
						continue;
					if(n2 == VK_S) {
						vt.clear();
						fcntl(vt.filedesc(),F_SETFL,0);
						return;
					}
				}
			}
			vt.clear();
			fcntl(vt.filedesc(),F_SETFL,0);

			cout << '\r';
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
