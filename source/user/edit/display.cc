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
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/sllist.h>
#include <esc/proto/vterm.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

#include "buffer.h"
#include "mem.h"
#include "display.h"

static void displ_updateLines(size_t start,size_t count);
static void displ_printStatus(void);

static esc::VTerm vterm(STDOUT_FILENO);
static esc::Screen::Mode mode;
static size_t firstLine = 0;
static size_t dirtyStart = 0;
static size_t dirtyCount;
static int curX = 0;
static int curXDispl = 0;
static int curXVirt = 0;
static int curXVirtDispl = 0;
static int curY = 0;
static sFileBuffer *buffer;

void displ_init(sFileBuffer *buf) {
	mode = vterm.getMode();

	/* one line for status-information :) */
	mode.rows--;
	buffer = buf;
	dirtyCount = mode.rows;

	/* backup screen */
	vterm.backup();
	/* stop readline and navigation */
	vterm.setFlag(esc::VTerm::FL_READLINE,false);
	vterm.setFlag(esc::VTerm::FL_NAVI,false);
}

void displ_finish(void) {
	printf("\n");
	/* ensure that the output is flushed before the vterm restores the old screen */
	fflush(stdout);
	vterm.setFlag(esc::VTerm::FL_READLINE,true);
	vterm.setFlag(esc::VTerm::FL_NAVI,true);
	vterm.restore();
}

void displ_getCurPos(int *col,int *row) {
	*col = curX;
	*row = firstLine + curY;
}

void displ_mvCurHor(uint type) {
	sLine *line;
	switch(type) {
		case HOR_MOVE_HOME:
			curXDispl = 0;
			curX = 0;
			break;
		case HOR_MOVE_END:
			line = (sLine*)sll_get(buffer->lines,firstLine + curY);
			curXDispl = line->displLen;
			curX = line->length;
			break;
		case HOR_MOVE_LEFT:
			if(curX == 0 && firstLine + curY > 0) {
				line = (sLine*)sll_get(buffer->lines,firstLine + curY - 1);
				displ_mvCurVert(-1);
				curXDispl = line->displLen;
				curX = line->length;
			}
			else if(curX > 0) {
				line = (sLine*)sll_get(buffer->lines,firstLine + curY);
				curXDispl -= displ_getCharLen(line->str[curX - 1]);
				curX--;
			}
			break;
		case HOR_MOVE_RIGHT:
			line = (sLine*)sll_get(buffer->lines,firstLine + curY);
			if(curX == (int)line->length && firstLine + curY < sll_length(buffer->lines)) {
				displ_mvCurVert(1);
				curXDispl = 0;
				curX = 0;
			}
			else if(curX < (int)line->length) {
				curXDispl += displ_getCharLen(line->str[curX]);
				curX++;
			}
			break;
	}
	curXVirt = curX;
	curXVirtDispl = curXDispl;
}

void displ_mvCurVertPage(bool up) {
	displ_mvCurVert(up ? -mode.rows : mode.rows);
}

void displ_mvCurVert(int lineCount) {
	size_t total = sll_length(buffer->lines);
	sLine *line;
	size_t oldFirst = firstLine;
	/* determine new y-position */
	if(lineCount > 0 && curY + lineCount < 0)
		curY = total - 1;
	else
		curY += lineCount;
	/* for files smaller than consSize.height */
	if(total < mode.rows && curY >= (int)total)
		curY = total - 1;
	else if(curY >= (int)mode.rows) {
		if(total >= mode.rows)
			firstLine = MIN(total - mode.rows,firstLine + (curY - mode.rows) + 1);
		else
			firstLine = MIN(total,firstLine + (curY - mode.rows) + 1);
		curY = mode.rows - 1;
	}
	else if(curY < 0) {
		firstLine = MAX(0,(int)firstLine + curY);
		curY = 0;
	}
	/* determine x-position */
	line = (sLine*)sll_get(buffer->lines,firstLine + curY);
	curX = MIN((int)line->length,MAX(curXVirt,curX));
	curXDispl = MIN((int)line->displLen,MAX(curXVirtDispl,curXDispl));
	/* anything to update? */
	if(oldFirst != firstLine)
		displ_markDirty(firstLine,mode.rows);
}

void displ_markDirty(size_t start,size_t count) {
	if(dirtyCount == 0) {
		dirtyStart = start;
		dirtyCount = count;
	}
	else {
		size_t oldstart = dirtyStart;
		if(start < oldstart)
			dirtyStart = start;
		dirtyCount = MAX(oldstart + dirtyCount,start + count) - dirtyStart;
	}
	dirtyCount = MIN(dirtyCount,mode.rows - (dirtyStart - firstLine));
}

void displ_update(void) {
	displ_updateLines(dirtyStart,dirtyCount);
	dirtyStart = ULONG_MAX;
	dirtyCount = 0;
}

size_t displ_getCharLen(char c) {
	return c == '\t' ? 2 : 1;
}

int displ_getSaveFile(char *file,size_t bufSize) {
	size_t i;
	char *res;
	printf("\033[go;%d;%d]",0,mode.rows - 1);
	for(i = 0; i < mode.cols; i++)
		putchar(' ');
	putchar('\r');
	vterm.setFlag(esc::VTerm::FL_READLINE,true);
	printf("\033[co;0;7]Save to file:\033[co] ");
	if(buffer->filename)
		printf("\033[si;1]%s\033[si;0]",buffer->filename);
	res = fgetl(file,bufSize,stdin);
	vterm.setFlag(esc::VTerm::FL_READLINE,false);
	displ_markDirty(firstLine + mode.rows - 1,1);
	displ_update();
	return res ? 0 : EOF;
}

static void displ_updateLines(size_t start,size_t count) {
	sSLNode *n;
	sLine *line;
	size_t i,j;
	assert(start >= firstLine);
	if(dirtyCount > 0) {
		printf("\033[go;0;%zd]",start - firstLine);
		if(start < sll_length(buffer->lines)) {
			for(n = sll_nodeAt(buffer->lines,start); n != NULL && count > 0; n = n->next, count--) {
				line = (sLine*)n->data;
				for(j = 0, i = 0; i < line->length && j < mode.cols; i++) {
					char c = line->str[i];
					switch(c) {
						case '\t':
							putchar(' ');
							putchar(' ');
							j += 2;
							break;
						case '\a':
						case '\b':
						case '\r':
							/* ignore */
							break;
						default:
							putchar(c);
							j++;
							break;
					}
				}
				for(; j < mode.cols; j++)
					putchar(' ');
			}
		}
		for(; count > 0; count--) {
			for(i = 0; i < mode.cols; i++)
				putchar(' ');
		}
		fflush(stdout);
	}
	displ_printStatus();
	printf("\033[go;%d;%d]",curXDispl,curY);
}

static void displ_printStatus(void) {
	size_t fileLen = buffer->filename ? strlen(buffer->filename) : 0;
	char *tmp = (char*)emalloc(mode.cols + 1);
	printf("\033[go;%d;%d]",0,mode.rows + 1);
	snprintf(tmp,mode.cols + 1,"Cursor @ %zd : %d",firstLine + curY + 1,curX + 1);
	printf("\033[co;0;7]%-*s%s%c\033[co]",
			(int)(mode.cols - fileLen - 1),tmp,
			buffer->filename ? buffer->filename : "",buffer->modified ? '*' : ' ');
	efree(tmp);
}
