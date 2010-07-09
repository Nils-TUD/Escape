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
#include <esc/io.h>
#include <stdio.h>
#include <esc/keycodes.h>
#include <esc/cmdargs.h>
#include <esc/mem/heap.h>
#include <esc/io/file.h>
#include <esc/io/console.h>
#include <esc/io/ifilestream.h>
#include <esc/exceptions/io.h>
#include <esc/exceptions/outofmemory.h>
#include <esc/util/vector.h>
#include <string.h>
#include <stdlib.h>
#include <esc/messages.h>
#include <esc/esccodes.h>

#define BUFFER_SIZE			1024
#define TAB_WIDTH			4

static void resetVterm(void);
static void readLines(u32 end);
static void scrollDown(s32 lines);
static void refreshScreen(void);
static void printStatus(const char *totalStr);
static void copy(char c);
static void newline(void);

static sIStream *vt;
static sIStream *in;
static bool seenEOF;
static u32 linePos;
static sVector *lines;
static char *filename;
static u32 startLine = 0;

static sVTSize consSize;
static char *emptyLine;

int main(int argc,char *argv[]) {
	char c;
	char vterm[MAX_PATH_LEN] = "/dev/";
	bool run = true;

	if((argc != 1 && argc != 2) || isHelpCmd(argc,argv)) {
		cerr->writef(cerr,"Usage: %s [<file>]\n",argv[0]);
		cerr->writef(cerr,"	navigation:\n");
		cerr->writef(cerr,"		up/down	- one line up/down\n");
		cerr->writef(cerr,"		pageup/pagedown - one page up/down\n");
		cerr->writef(cerr,"		home/end - to the very beginning or end\n");
		cerr->writef(cerr,"		q - quit\n");
		cerr->writef(cerr,"		s - stop reading (e.g. when walking to EOF)\n");
		return EXIT_FAILURE;
	}

	/* determine source */
	in = cin;
	filename = NULL;
	TRY {
		if(argc == 2) {
			sFile *f = file_get(argv[1]);
			filename = (char*)heap_alloc(MAX_PATH_LEN);
			f->getAbsolute(f,filename,MAX_PATH_LEN);
			if(f->isDir(f))
				error("'%s' is a directory",filename);
			in = ifstream_open(filename,IO_READ);
			f->destroy(f);
		}
		else if(isterm(STDIN_FILENO))
			error("Using a vterm as STDIN and have got no filename");
	}
	CATCH(IOException,e) {
		error("Unable to open '%s' for reading: %s",argv[1],e->toString(e));
	}
	ENDCATCH

	if(recvMsgData(STDOUT_FILENO,MSG_VT_GETSIZE,&consSize,sizeof(sVTSize)) < 0)
		error("Unable to get screensize");
	/* one line for the status */
	consSize.height--;

	/* backup screen */
	send(STDOUT_FILENO,MSG_VT_BACKUP,NULL,0);

	/* create empty line */
	emptyLine = (char*)heap_alloc(consSize.width + 1);
	memset(emptyLine,' ',consSize.width);
	emptyLine[consSize.width] = '\0';

	/* open the "real" stdin, because stdin maybe redirected to something else */
	if(!getenvto(vterm + SSTRLEN("/dev/"),MAX_PATH_LEN - SSTRLEN("/dev/"),"TERM")) {
		resetVterm();
		error("Unable to get TERM");
	}
	vt = ifstream_open(vterm,IO_READ);

	TRY {
		/* create line-buffer and read the first lines into it */
		seenEOF = false;
		lines = vec_create(sizeof(char*));
		newline();
		readLines(consSize.height);

		/* stop readline and navigation */
		send(STDOUT_FILENO,MSG_VT_DIS_RDLINE,NULL,0);
		send(STDOUT_FILENO,MSG_VT_DIS_NAVI,NULL,0);

		refreshScreen();

		/* read from vterm */
		while(run && (c = vt->readc(vt)) != EOF) {
			if(c == '\033') {
				s32 n1,n2,n3;
				s32 cmd = vt->readEsc(vt,&n1,&n2,&n3);
				if(cmd != ESCC_KEYCODE)
					continue;
				switch(n2) {
					case VK_Q:
						run = false;
						break;
					case VK_HOME:
						/* scrollDown(0) means scroll to end.. */
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
	}
	CATCH(OutOfMemoryException,e) {
		resetVterm();
		error("Not enough memory");
	}
	ENDCATCH

	heap_free(emptyLine);
	resetVterm();
	vt->close(vt);
	in->close(in);

	return EXIT_SUCCESS;
}

static void resetVterm(void) {
	cout->writeln(cout,"");
	cout->flush(cout);
	send(STDOUT_FILENO,MSG_VT_EN_RDLINE,NULL,0);
	send(STDOUT_FILENO,MSG_VT_EN_NAVI,NULL,0);
	send(STDOUT_FILENO,MSG_VT_RESTORE,NULL,0);
}

static void scrollDown(s32 l) {
	u32 oldStart = startLine;
	if(l < 0) {
		if((s32)startLine + (s32)l >= 0)
			startLine += l;
		else
			startLine = 0;
	}
	else if(lines->count >= consSize.height)
		startLine += l;

	if(l == 0 || startLine > lines->count - consSize.height) {
		readLines(l == 0 ? 0 : startLine + consSize.height);
		if(lines->count < consSize.height)
			startLine = 0;
		else if(l == 0 || startLine > lines->count - consSize.height)
			startLine = lines->count - consSize.height;
	}

	if(oldStart != startLine)
		refreshScreen();
}

static void refreshScreen(void) {
	u32 i,end = MIN(lines->count,consSize.height);
	/* walk to the top of the screen */
	cout->writes(cout,"\033[mh]");
	sIterator it = vec_iteratorIn(lines,startLine,end);
	for(i = 0; it.hasNext(&it); i++) {
		char *line = (char*)it.next(&it);
		cout->writes(cout,line);
		if(i < consSize.height - 1)
			cout->writec(cout,'\n');
	}
	for(; i < consSize.height; i++) {
		cout->writes(cout,emptyLine);
		if(i < consSize.height - 1)
			cout->writec(cout,'\n');
	}
	printStatus(seenEOF ? NULL : "?");
	cout->flush(cout);
}

static void printStatus(const char *totalStr) {
	char *tmp;
	const char *displayName;
	u32 end = MIN(lines->count,consSize.height);
	tmp = (char*)heap_alloc(consSize.width + 1);
	if(!totalStr)
		snprintf(tmp,consSize.width + 1,"Lines %d-%d / %d",startLine + 1,startLine + end,lines->count);
	else
		snprintf(tmp,consSize.width + 1,"Lines %d-%d / %s",startLine + 1,startLine + end,totalStr);
	displayName = (filename == NULL) ? "STDIN" : filename;
	cout->writef(cout,"\033[co;0;7]%-*s%s\033[co]",consSize.width - strlen(displayName),tmp,displayName);
	heap_free(tmp);
}

static void readLines(u32 end) {
	const char *states[] = {"|","/","-","\\","|","/","-"};
	s32 count = 0;
	u8 state = 0;
	bool waitForEsc;
	char c,*cpy,*buffer;

	/* read and split into lines */
	buffer = (char*)heap_alloc(BUFFER_SIZE);
	waitForEsc = false;
	while((end == 0 || lines->count < end) && (count = in->read(in,buffer,BUFFER_SIZE - 1)) > 0) {
		*(buffer + count) = '\0';
		cpy = buffer;
		while((c = *cpy)) {
			/* skip escape-codes */
			if(c == '\033' || waitForEsc) {
				waitForEsc = true;
				while((c = *cpy) != ']' && c)
					cpy++;
				if(!c)
					break;
				waitForEsc = false;
			}
			else
				copy(c);
			cpy++;
		}

		/* check whether the user has pressed a key */
		while(!vt->eof(vt) && (c = vt->readc(vt)) != EOF) {
			if(c == '\033') {
				s32 n1,n2,n3;
				s32 cmd = vt->readEsc(vt,&n1,&n2,&n3);
				if(cmd != ESCC_KEYCODE)
					continue;
				if(n2 == VK_S)
					goto finished;
			}
		}

		cout->writec(cout,'\r');
		printStatus(states[state]);
		state = (state + 1) % ARRAY_SIZE(states);
	}
	if(count <= 0)
		seenEOF = true;
finished:
	heap_free(buffer);
}

static void copy(char c) {
	char *pos;
	/* implicit newline? */
	if(c != '\n' && linePos >= consSize.width)
		copy('\n');

	pos = (char*)vec_get(lines,lines->count - 1) + linePos;
	switch(c) {
		case '\t': {
			u32 i;
			for(i = TAB_WIDTH - linePos % TAB_WIDTH; i > 0; i--)
				copy(' ');
		}
		break;

		case '\n':
			linePos = 0;
			newline();
			break;

		/* ignore */
		case '\b':
		case '\a':
		case '\r':
			break;

		default:
			*pos = c;
			linePos++;
			break;
	}
}

static void newline(void) {
	char *line = (char*)heap_alloc(consSize.width + 1);
	vec_add(lines,&line);
	/* fill the line with spaces */
	memset(line,' ',consSize.width);
	line[consSize.width] = '\0';
}
