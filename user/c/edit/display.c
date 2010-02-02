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
#include <esc/fileio.h>
#include <assert.h>
#include <limits.h>
#include <sllist.h>
#include <string.h>
#include "buffer.h"
#include "mem.h"
#include "display.h"

static void displ_updateLines(u32 start,u32 count);
static void displ_printStatus(void);

static sIoCtlSize consSize;
static u32 firstLine = 0;
static u32 dirtyStart = 0;
static u32 dirtyCount;
static s32 curX = 0;
static s32 curXDispl = 0;
static s32 curXVirt = 0;
static s32 curXVirtDispl = 0;
static s32 curY = 0;
static sFileBuffer *buffer;

void displ_init(sFileBuffer *buf) {
	if(ioctl(STDOUT_FILENO,IOCTL_VT_GETSIZE,&consSize,sizeof(sIoCtlSize)) < 0)
		error("Unable to get screensize");

	/* one line for status-information :) */
	consSize.height--;
	buffer = buf;
	dirtyCount = consSize.height;

	/* backup screen */
	ioctl(STDOUT_FILENO,IOCTL_VT_BACKUP,NULL,0);
	/* stop readline and navigation */
	ioctl(STDOUT_FILENO,IOCTL_VT_DIS_RDLINE,NULL,0);
	ioctl(STDOUT_FILENO,IOCTL_VT_DIS_NAVI,NULL,0);
}

void displ_finish(void) {
	printf("\n");
	ioctl(STDOUT_FILENO,IOCTL_VT_EN_RDLINE,NULL,0);
	ioctl(STDOUT_FILENO,IOCTL_VT_EN_NAVI,NULL,0);
	ioctl(STDOUT_FILENO,IOCTL_VT_RESTORE,NULL,0);
}

void displ_getCurPos(s32 *col,s32 *row) {
	*col = curX;
	*row = firstLine + curY;
}

void displ_mvCurHor(u8 type) {
	sLine *line;
	switch(type) {
		case HOR_MOVE_HOME:
			curXDispl = 0;
			curX = 0;
			break;
		case HOR_MOVE_END:
			line = sll_get(buffer->lines,firstLine + curY);
			curXDispl = line->displLen;
			curX = line->length;
			break;
		case HOR_MOVE_LEFT:
			if(curX == 0 && firstLine + curY > 0) {
				line = sll_get(buffer->lines,firstLine + curY - 1);
				displ_mvCurVert(-1);
				curXDispl = line->displLen;
				curX = line->length;
			}
			else if(curX > 0) {
				line = sll_get(buffer->lines,firstLine + curY);
				curXDispl -= displ_getCharLen(line->str[curX - 1]);
				curX--;
			}
			break;
		case HOR_MOVE_RIGHT:
			line = sll_get(buffer->lines,firstLine + curY);
			if(curX == (s32)line->length && firstLine + curY < sll_length(buffer->lines)) {
				displ_mvCurVert(1);
				curXDispl = 0;
				curX = 0;
			}
			else if(curX < (s32)line->length) {
				curXDispl += displ_getCharLen(line->str[curX]);
				curX++;
			}
			break;
	}
	curXVirt = curX;
	curXVirtDispl = curXDispl;
}

void displ_mvCurVertPage(bool up) {
	displ_mvCurVert(up ? -consSize.height : consSize.height);
}

void displ_mvCurVert(s32 lineCount) {
	u32 total = sll_length(buffer->lines);
	sLine *line;
	u32 oldFirst = firstLine;
	/* determine new y-position */
	if(lineCount > 0 && curY + lineCount < 0)
		curY = total - 1;
	else
		curY += lineCount;
	/* for files smaller than consSize.height */
	if(total < consSize.height && curY >= (s32)total)
		curY = total - 1;
	else if(curY >= (s32)consSize.height) {
		if(total >= consSize.height)
			firstLine = MIN(total - consSize.height,firstLine + (curY - consSize.height) + 1);
		else
			firstLine = MIN(total,firstLine + (curY - consSize.height) + 1);
		curY = consSize.height - 1;
	}
	else if(curY < 0) {
		firstLine = MAX(0,(s32)firstLine + curY);
		curY = 0;
	}
	/* determine x-position */
	line = sll_get(buffer->lines,firstLine + curY);
	curX = MIN((s32)line->length,MAX(curXVirt,curX));
	curXDispl = MIN((s32)line->displLen,MAX(curXVirtDispl,curXDispl));
	/* anything to update? */
	if(oldFirst != firstLine)
		displ_markDirty(firstLine,consSize.height);
}

void displ_markDirty(u32 start,u32 count) {
	if(dirtyCount == 0) {
		dirtyStart = start;
		dirtyCount = count;
	}
	else {
		u32 oldstart = dirtyStart;
		if(start < oldstart)
			dirtyStart = start;
		dirtyCount = MAX(oldstart + dirtyCount,start + count) - dirtyStart;
	}
	dirtyCount = MIN(dirtyCount,consSize.height - (dirtyStart - firstLine));
}

void displ_update(void) {
	displ_updateLines(dirtyStart,dirtyCount);
	dirtyStart = ULONG_MAX;
	dirtyCount = 0;
}

u32 displ_getCharLen(char c) {
	return c == '\t' ? 2 : 1;
}

static void displ_updateLines(u32 start,u32 count) {
	sSLNode *n;
	sLine *line;
	u32 i,j;
	assert(start >= firstLine);
	if(dirtyCount > 0) {
		printf("\033[go;0;%d]",start - firstLine);
		for(n = sll_nodeAt(buffer->lines,start); n != NULL && count > 0; n = n->next, count--) {
			line = (sLine*)n->data;
			for(j = 0, i = 0; i < line->length && j < consSize.width; i++) {
				char c = line->str[i];
				switch(c) {
					case '\t':
						printc(' ');
						printc(' ');
						j += 2;
						break;
					case '\a':
					case '\b':
					case '\r':
						/* ignore */
						break;
					default:
						printc(c);
						j++;
						break;
				}
			}
			for(; j < consSize.width; j++)
				printc(' ');
		}
		for(; count > 0; count--) {
			for(i = 0; i < consSize.width; i++)
				printc(' ');
		}
		flush();
	}
	printf("\033[go;%d;%d]",0,consSize.height + 1);
	displ_printStatus();
	printf("\033[go;%d;%d]",curXDispl,curY);
}

static void displ_printStatus(void) {
	u32 fileLen = strlen(buffer->filename);
	char *tmp = (char*)emalloc(consSize.width + 1);
	sprintf(tmp,"Cursor @ %d : %d",firstLine + curY + 1,curX + 1);
	printf("\033[co;0;7]%-*s%s%c\033[co]",
			consSize.width - fileLen - 1,tmp,buffer->filename,buffer->modified ? '*' : ' ');
	efree(tmp);
}
