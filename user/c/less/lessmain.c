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
#include <esc/messages.h>
#include <stdlib.h>

#define COLS				80
#define ROWS				24
#define BUFFER_SIZE			(64 * K)
#define BUFFER_INC_SIZE		64
#define TAB_WIDTH			4

static bool readLines(tFile *file);
static void scrollDown(s32 lines);
static void refreshScreen(void);
static bool copy(char c);

static u32 linePos;
static u32 lineCount;
static u32 lineSize;
static char **lines;
static u32 startLine = 0;

/* vterm-command to walk to the top of the screen */
static char emptyLine[COLS];
static char walkBack[(3 * ROWS + 1) * sizeof(char)];

int main(int argc,char *argv[]) {
	tFile *file;
	tFile *fvterm;
	u32 i;
	char c;
	char *path;
	char vterm[MAX_PATH_LEN] = "services:/";

	if(argc != 1 && argc != 2) {
		fprintf(stderr,"Usage: %s [<file>]\n",argv[0]);
		return EXIT_FAILURE;
	}

	/* determine source */
	file = stdin;
	if(argc == 2) {
		path = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
		if(path == NULL) {
			printe("Unable to allocate mem for path");
			return EXIT_FAILURE;
		}

		abspath(path,MAX_PATH_LEN + 1,argv[1]);
		file = fopen(path,"r");
		if(file == NULL) {
			printe("Unable to open '%s'",path);
			return EXIT_FAILURE;
		}

		free(path);
	}

	/* stop readline and backup screen */
	printf("\033l\x0\033c\x0");

	/* read all */
	if(!readLines(file)) {
		printf("\033l\x1\033t\x0");
		return EXIT_FAILURE;
	}

	if(argc == 2)
		fclose(file);

	/* open the "real" stdin, because stdin maybe redirected to something else */
	if(!getEnv(vterm + 10,MAX_PATH_LEN - 10,"TERM")) {
		printf("\033l\x1\033t\x0");
		printe("Unable to get TERM");
		return EXIT_FAILURE;
	}
	fvterm = fopen(vterm,"r");
	if(fvterm == NULL) {
		printf("\033l\x1\033t\x0");
		printe("Unable to open '%s'",vterm);
		return EXIT_FAILURE;
	}

	/* init walkback-string */
	for(i = 0; i < ROWS * 3; i += 3) {
		walkBack[i] = '\033';
		walkBack[i + 1] = VK_UP;
		walkBack[i + 2] = 0;
	}
	walkBack[ROWS * 3] = '\0';
	/* init empty line */
	memset(emptyLine,' ',COLS - 1);
	emptyLine[COLS - 1] = '\0';

	refreshScreen();

	/* read from vterm */
	while((c = fscanc(fvterm)) != IO_EOF) {
		if(c == 'q')
			break;
		if(c == '\033') {
			u8 keycode = fscanc(fvterm);
			u8 modifier = fscanc(fvterm);
			switch(keycode) {
				case VK_UP:
					scrollDown(-1);
					break;
				case VK_DOWN:
					scrollDown(1);
					break;
				case VK_PGUP:
					scrollDown(-ROWS);
					break;
				case VK_PGDOWN:
					scrollDown(ROWS);
					break;
			}
		}
	}

	fclose(fvterm);

	/* reenable readline and restore the screen */
	printf("\033l\x1\033t\x0\r");

	return EXIT_SUCCESS;
}

static void scrollDown(s32 l) {
	u32 oldStart = startLine;
	if(l < 0) {
		if((s32)startLine + (s32)l >= 0)
			startLine += l;
		else
			startLine = 0;
	}
	else if(lineCount >= ROWS) {
		if(startLine + l < lineCount - ROWS)
			startLine += l;
		else
			startLine = lineCount - ROWS;
	}
	if(oldStart != startLine)
		refreshScreen();
}

static void refreshScreen(void) {
	/* walk to the top of the screen */
	prints(walkBack);
	printc('\r');
	flush();
	u32 i,end = MIN(lineCount,ROWS);
	for(i = 0; i < end; i++) {
		prints(lines[startLine + i]);
		if(i < ROWS - 1)
			printc('\n');
	}
	for(; i < ROWS; i++) {
		prints(emptyLine);
		if(i < ROWS - 1)
			printc('\n');
	}
	flush();
}

static bool readLines(tFile *file) {
	s32 count;
	char *cpy;
	char *buffer;

	/* create line-buffer */
	lineCount = 0;
	lineSize = BUFFER_INC_SIZE;
	lines = (char**)calloc(lineSize,sizeof(char*));
	if(lines == NULL) {
		printe("Unable to create lines");
		return false;
	}

	/* create buffer for reading */
	buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
	if(buffer == NULL) {
		printe("Unable to allocate mem");
		return false;
	}

	/* read and split into lines */
	while((count = fread(buffer,sizeof(char),BUFFER_SIZE - 1,file)) > 0) {
		*(buffer + count) = '\0';
		cpy = buffer;
		while(*cpy) {
			/* skip escape-codes */
			if(*cpy == '\033')
				cpy += 2;
			else if(!copy(*cpy)) {
				free(buffer);
				return false;
			}
			cpy++;
		}
	}
	free(buffer);
	return true;
}

static bool copy(char c) {
	/* implicit newline? */
	if(linePos >= COLS - 1) {
		if(!copy('\n'))
			return false;
	}

	/* line not yet present? */
	if(lines[lineCount] == NULL) {
		lines[lineCount] = (char*)malloc(COLS * sizeof(char));
		if(lines[lineCount] == NULL) {
			printe("Unable to allocate mem");
			return false;
		}
		/* fill the line with spaces */
		memset(lines[lineCount],' ',COLS - 1);
		/* terminate */
		lines[lineCount][COLS - 1] = '\0';
	}

	char *pos = lines[lineCount] + linePos;
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
