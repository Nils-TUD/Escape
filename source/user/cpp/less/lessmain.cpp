/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <esc/common.h>
#include <esc/keycodes.h>
#include <esc/messages.h>
#include <esc/esccodes.h>
#include <esc/dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmdargs.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>
#include <env.h>

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
/* for now it works when we make sure that the size of all stuff here together is a multiple of 4 */

static ifstream vt;
static FILE *in;
static string filename;
static int seenEOF;
static vector<string> lines;
static size_t startLine = 0;
static sVTSize consSize;
static string emptyLine;

static void usage(const char* name) {
	cerr << "Usage: " << name << " [<file>]" << endl;
	cerr << "	navigation:" << endl;
	cerr << "		up/down	- one line up/down" << endl;
	cerr << "		pageup/pagedown - one page up/down" << endl;
	cerr << "		home/end - to the very beginning or end" << endl;
	cerr << "		q - quit" << endl;
	cerr << "		s - stop reading (e.g. when walking to EOF)" << endl;
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
		cerr << "Invalid arguments: " << e.what() << endl;
		usage(argv[0]);
	}

	// open file or use stdin
	if(args.get_free().size() > 0) {
		filename = *args.get_free().at(0);
		in = fopen(filename.c_str(),"r");
		if(in == NULL)
			error("Unable to open '%s'",filename.c_str());
	}
	else if(isterm(STDIN_FILENO))
		error("Using a vterm as STDIN and have got no filename");
	else {
		filename = "STDIN";
		in = stdin;
	}

	if(recvMsgData(STDOUT_FILENO,MSG_VT_GETSIZE,&consSize,sizeof(sVTSize)) < 0)
		error("Unable to get screensize");
	// one line for the status
	consSize.height--;

	// backup screen
	sendRecvMsgData(STDOUT_FILENO,MSG_VT_BACKUP,NULL,0);

	// create empty line
	emptyLine.assign(consSize.width,' ');

	/* open the "real" stdin, because stdin maybe redirected to something else */
	string vtermPath = string("/dev/") + env::get("TERM");
	vt.open(vtermPath.c_str());

	// read the first lines into it
	seenEOF = false;
	readLines(consSize.height);

	// stop readline and navigation
	sendRecvMsgData(STDOUT_FILENO,MSG_VT_DIS_RDLINE,NULL,0);
	sendRecvMsgData(STDOUT_FILENO,MSG_VT_DIS_NAVI,NULL,0);

	refreshScreen();

	// read from vterm
	while(run && (c = vt.get()) != EOF) {
		if(c == '\033') {
			istream::esc_type n1,n2,n3;
			istream::esc_type cmd = vt.getesc(n1,n2,n3);
			if(cmd != ESCC_KEYCODE)
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
					scrollDown(-consSize.height);
					break;
				case VK_PGDOWN:
					scrollDown(consSize.height);
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
	cout << endl;
	sendRecvMsgData(STDOUT_FILENO,MSG_VT_EN_RDLINE,NULL,0);
	sendRecvMsgData(STDOUT_FILENO,MSG_VT_EN_NAVI,NULL,0);
	sendRecvMsgData(STDOUT_FILENO,MSG_VT_RESTORE,NULL,0);
}

static void scrollDown(long l) {
	size_t oldStart = startLine;
	if(l < 0) {
		if((long)startLine + l >= 0)
			startLine += l;
		else
			startLine = 0;
	}
	else if(lines.size() >= consSize.height)
		startLine += l;

	if(l == 0 || startLine > lines.size() - consSize.height) {
		readLines(l == 0 ? 0 : startLine + consSize.height);
		if(lines.size() < consSize.height)
			startLine = 0;
		else if(l == 0 || startLine > lines.size() - consSize.height)
			startLine = lines.size() - consSize.height;
	}

	if(oldStart != startLine)
		refreshScreen();
}

static void refreshScreen(void) {
	vector<string>::const_iterator begin = lines.begin() + startLine;
	vector<string>::const_iterator end = begin + min(lines.size(),(size_t)consSize.height);
	size_t i = 0;
	// walk to the top of the screen
	cout << "\033[mh]";
	for(vector<string>::const_iterator it = begin; it != end; ++it ,++i) {
		cout << *it;
		if(i < consSize.height - 1)
			cout << '\n';
	}
	for(; i < consSize.height; ++i) {
		cout << emptyLine;
		if(i < consSize.height - 1)
			cout << '\n';
	}
	printStatus(seenEOF ? NULL : "?");
	cout.flush();
}

static void printStatus(const char *totalStr) {
	string line;
	size_t end = min(lines.size(),(size_t)consSize.height);
	if(!totalStr)
		line.format("Lines %d-%d / %d",startLine + 1,startLine + end,lines.size());
	else
		line.format("Lines %d-%d / %s",startLine + 1,startLine + end,totalStr);
	cout << "\033[co;0;7]";
	cout << line << setw(consSize.width - line.length()) << right << filename;
	cout << left << "\033[co]";
}

static void readLines(size_t end) {
	int lineCount = 0;
	int lineLen = consSize.width;
	string line;
	static const char *states[] = {"|","/","-","\\","|","/","-"};
	int state = 0;
	if(seenEOF)
		return;

	// read
	char c = 0;
	while(c != EOF && (end == 0 || lines.size() < end)) {
		int len = 0;
		line.clear();
		while((c = fgetc(in)) != EOF && c != '\n') {
			if(len < lineLen)
				append(line,len,c);
		}
		line.resize((line.length() - len) + lineLen,' ');
		lines.push_back(string(line));

		// check whether the user has pressed a key
		if(lineCount++ % 100 == 0) {
			char vtc;
			fcntl(vt.filedesc(),F_SETFL,IO_NOBLOCK);
			while((vtc = vt.get()) != EOF) {
				if(vtc == '\033') {
					istream::esc_type n1,n2,n3;
					istream::esc_type cmd = vt.getesc(n1,n2,n3);
					if(cmd != ESCC_KEYCODE)
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
			s += c;
			if(inEsc) {
				if(c == ']')
					inEsc = 0;
			}
			else if(c == '\033')
				inEsc = 1;
			else
				len++;
			break;
	}
}
