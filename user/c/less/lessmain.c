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
#include <esc/dir.h>
#include <esc/fileio.h>
#include <esc/heap.h>
#include <esc/io.h>
#include <esc/keycodes.h>
#include <esc/cmdargs.h>
#include <esc/env.h>
#include <messages.h>
#include <esccodes.h>
#include <stdlib.h>

#define BUFFER_SIZE			1024
#define BUFFER_INC_SIZE		64
#define TAB_WIDTH			4

static void resetVterm(void);
static bool readLines(u32 end);
static void scrollDown(s32 lines);
static void refreshScreen(void);
static void printStatus(u32 total,const char *totalStr);
static bool copy(char c);

static tFile *fvterm;
static tFile *file;
static bool seenEOF;
static u32 linePos;
static u32 lineCount;
static u32 lineSize;
static char **lines;
static char *filename;
static u32 startLine = 0;

static sIoCtlSize consSize;
static char *emptyLine;

int main(int argc,char *argv[]) {
	bool run = true;
	char c;
	char vterm[MAX_PATH_LEN] = "/drivers/";

	if((argc != 1 && argc != 2) || isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s [<file>]\n",argv[0]);
		fprintf(stderr,"	navigation:\n");
		fprintf(stderr,"		up/down	- one line up/down\n");
		fprintf(stderr,"		pageup/pagedown - one page up/down\n");
		fprintf(stderr,"		home/end - to the very beginning or end\n");
		fprintf(stderr,"		q - quit\n");
		fprintf(stderr,"		s - stop reading (e.g. when walking to EOF)\n");
		return EXIT_FAILURE;
	}

	/* determine source */
	file = stdin;
	filename = NULL;
	if(argc == 2) {
		filename = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
		if(filename == NULL)
			error("Unable to allocate mem for path");

		abspath(filename,MAX_PATH_LEN + 1,argv[1]);
		file = fopen(filename,"r");
		if(file == NULL)
			error("Unable to open '%s'",filename);
	}

	if(ioctl(STDOUT_FILENO,IOCTL_VT_GETSIZE,&consSize,sizeof(sIoCtlSize)) < 0)
		error("Unable to get screensize");
	/* one line for the status */
	consSize.height--;

	/* backup screen */
	ioctl(STDOUT_FILENO,IOCTL_VT_BACKUP,NULL,0);

	/* create empty line */
	emptyLine = (char*)malloc(consSize.width + 1);
	if(emptyLine == NULL)
		error("Unable to alloc memory for empty line");
	memset(emptyLine,' ',consSize.width);
	emptyLine[consSize.width] = '\0';

	/* create line-buffer */
	seenEOF = false;
	lineCount = 0;
	lineSize = BUFFER_INC_SIZE;
	lines = (char**)calloc(lineSize,sizeof(char*));
	if(lines == NULL)
		error("Unable to create lines");

	/* read all */
	if(!readLines(consSize.height)) {
		resetVterm();
		return EXIT_FAILURE;
	}

	/* stop readline and navigation */
	ioctl(STDOUT_FILENO,IOCTL_VT_DIS_RDLINE,NULL,0);
	ioctl(STDOUT_FILENO,IOCTL_VT_DIS_NAVI,NULL,0);

	/* open the "real" stdin, because stdin maybe redirected to something else */
	if(!getEnv(vterm + 9,MAX_PATH_LEN - 9,"TERM")) {
		resetVterm();
		error("Unable to get TERM");
	}
	fvterm = fopen(vterm,"r");
	if(fvterm == NULL) {
		resetVterm();
		error("Unable to open '%s'",vterm);
	}

	refreshScreen();

	/* read from vterm */
	while(run && (c = fscanc(fvterm)) != IO_EOF) {
		if(c == '\033') {
			s32 n1,n2,n3;
			s32 cmd = freadesc(fvterm,&n1,&n2,&n3);
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

	free(emptyLine);
	if(argc == 2)
		fclose(file);
	fclose(fvterm);
	resetVterm();

	return EXIT_SUCCESS;
}

static void resetVterm(void) {
	printf("\n");
	flush();
	ioctl(STDOUT_FILENO,IOCTL_VT_EN_RDLINE,NULL,0);
	ioctl(STDOUT_FILENO,IOCTL_VT_EN_NAVI,NULL,0);
	ioctl(STDOUT_FILENO,IOCTL_VT_RESTORE,NULL,0);
}

static void scrollDown(s32 l) {
	u32 oldStart = startLine;
	if(l < 0) {
		if((s32)startLine + (s32)l >= 0)
			startLine += l;
		else
			startLine = 0;
	}
	else if(lineCount >= consSize.height)
		startLine += l;

	if(l == 0 || startLine > lineCount - consSize.height) {
		readLines(l == 0 ? 0 : startLine + consSize.height);
		if(lineCount < consSize.height)
			startLine = 0;
		else
			startLine = lineCount - consSize.height;
	}

	if(oldStart != startLine)
		refreshScreen();
}

static void refreshScreen(void) {
	u32 i,end = MIN(lineCount,consSize.height);
	/* walk to the top of the screen */
	printf("\033[mh]");
	for(i = 0; i < end; i++) {
		prints(lines[startLine + i]);
		if(i < consSize.height - 1)
			printc('\n');
	}
	for(; i < consSize.height; i++) {
		prints(emptyLine);
		if(i < consSize.height - 1)
			printc('\n');
	}
	printStatus(lineCount,seenEOF ? NULL : "?");
	flush();
}

static void printStatus(u32 total,const char *totalStr) {
	char *tmp;
	const char *displayName;
	u32 end = MIN(lineCount,consSize.height);
	tmp = (char*)malloc(consSize.width + 1);
	if(tmp == NULL)
		return;
	if(!totalStr)
		sprintf(tmp,"Lines %d-%d / %d",startLine + 1,startLine + end,total);
	else
		sprintf(tmp,"Lines %d-%d / %s",startLine + 1,startLine + end,totalStr);
	if(filename == NULL)
		displayName = "STDIN";
	else
		displayName = filename;
	printf("\033[co;0;7]%-*s%s\033[co]",consSize.width - strlen(displayName),tmp,displayName);
	free(tmp);
}

static bool readLines(u32 end) {
	const char *states[] = {"|","/","-","\\","|","/","-"};
	s32 count;
	u8 state = 0;
	bool waitForEsc;
	char *cpy;
	char *buffer;

	/* create buffer for reading */
	buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
	if(buffer == NULL) {
		printe("Unable to allocate mem");
		return false;
	}

	/* read and split into lines */
	waitForEsc = false;
	while((end == 0 || lineCount < end) &&
			(count = fread(buffer,sizeof(char),BUFFER_SIZE - 1,file)) > 0) {
		*(buffer + count) = '\0';
		cpy = buffer;
		while(*cpy) {
			if(*cpy == IO_EOF) {
				copy('\n');
				seenEOF = true;
				goto finished;
			}
			/* skip escape-codes */
			if(*cpy == '\033' || waitForEsc) {
				waitForEsc = true;
				while(*cpy != ']' && *cpy)
					cpy++;
				if(!*cpy)
					break;
				waitForEsc = false;
			}
			else if(!copy(*cpy)) {
				free(buffer);
				return false;
			}
			cpy++;
		}

		/* check wether the user has pressed a key */
		{
			char c;
			while(!feof(fvterm) && (c = fscanc(fvterm)) != IO_EOF) {
				if(c == '\033') {
					s32 n1,n2,n3;
					s32 cmd = freadesc(fvterm,&n1,&n2,&n3);
					if(cmd != ESCC_KEYCODE)
						continue;
					if(n2 == VK_S)
						goto finished;
				}
			}
		}

		printf("\r");
		printStatus(lineCount,states[state]);
		state = (state + 1) % ARRAY_SIZE(states);
	}
	if(count <= 0)
		seenEOF = true;
finished:
	free(buffer);
	return true;
}

static bool copy(char c) {
	char *pos;
	/* implicit newline? */
	if(c != '\n' && linePos >= consSize.width) {
		if(!copy('\n'))
			return false;
	}

	/* line not yet present? */
	if(lines[lineCount] == NULL) {
		lines[lineCount] = (char*)malloc((consSize.width + 1) * sizeof(char));
		if(lines[lineCount] == NULL) {
			printe("Unable to allocate mem");
			return false;
		}
		/* fill the line with spaces */
		memset(lines[lineCount],' ',consSize.width);
		/* terminate */
		lines[lineCount][consSize.width] = '\0';
	}

	pos = lines[lineCount] + linePos;
	switch(c) {
		case '\t': {
			u32 i;
			for(i = TAB_WIDTH - linePos % TAB_WIDTH; i > 0; i--) {
				if(!copy(' '))
					return false;
			}
		}
		break;

		case '\n':
			linePos = 0;
			lineCount++;
			/* line-buffer full? */
			if(lineCount >= lineSize) {
				lineSize += BUFFER_INC_SIZE;
				lines = (char**)realloc(lines,lineSize * sizeof(char*));
				/* ensure that all pointers are NULL */
				memclear(lines + lineSize - BUFFER_INC_SIZE,BUFFER_INC_SIZE * sizeof(char*));
				if(lines == NULL) {
					printe("Unable to reallocate lines");
					return false;
				}
			}
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
	return true;
}
