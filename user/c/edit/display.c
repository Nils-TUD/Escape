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
#include <sllist.h>
#include "buffer.h"
#include "display.h"

static void displ_updateLines(sSLList *lines,u32 start,u32 count);

static sIoCtlSize consSize;
static u32 firstLine;
static u32 dirtyStart;
static u32 dirtyCount;

void displ_init(void) {
	if(ioctl(STDOUT_FILENO,IOCTL_VT_GETSIZE,&consSize,sizeof(sIoCtlSize)) < 0)
		error("Unable to get screensize");

	firstLine = 0;
	dirtyStart = 0;
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

void displ_update(sSLList *lines) {
	displ_updateLines(lines,dirtyStart,dirtyCount);
}

static void displ_updateLines(sSLList *lines,u32 start,u32 count) {
	sSLNode *n;
	sLine *line;
	u32 i,end;
	assert(start >= firstLine);
	printf("\033[go;0;%d]",start - firstLine);
	for(n = sll_nodeAt(lines,start); n != NULL && count > 0; n = n->next, count--) {
		line = (sLine*)n->data;
		end = MIN(consSize.width,line->length);
		for(i = 0; i < end; i++)
			printc(line->str[i]);
		for(; i < consSize.width; i++)
			printc(' ');
	}
	for(; count > 0; count--) {
		for(i = 0; i < consSize.width; i++)
			printc(' ');
	}
	flush();
}
