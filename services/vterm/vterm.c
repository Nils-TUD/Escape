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
#include <messages.h>
#include <esc/io.h>
#include <esc/ports.h>
#include <esc/proc.h>
#include <esc/keycodes.h>
#include <esc/heap.h>
#include <esc/signals.h>
#include <esc/service.h>
#include <esc/fileio.h>
#include <esc/lock.h>
#include <esc/thread.h>
#include <esc/date.h>
#include <stdlib.h>
#include <string.h>
#include <errors.h>
#include <ringbuffer.h>

#include "vterm.h"

/* the number of chars to keep in history */
#define INITIAL_RLBUF_SIZE	50

#define TITLE_BAR_COLOR		0x17
#define OS_TITLE			"E\x17" \
							"s\x17" \
							"c\x17" \
							"a\x17" \
							"p\x17" \
							"e\x17" \
							" \x17" \
							"v\x17" \
							"0\x17" \
							".\x17" \
							"1\x17"

/**
 * Inits the vterm
 *
 * @param vt the vterm
 * @return true if successfull
 */
static bool vterm_init(sVTerm *vt);

/**
 * The thread that updates the titlebars every second and puts the date in
 */
static int vterm_dateThread(void);

/* vterms */
static tULock titleBarLock;
static sVTerm vterms[VTERM_COUNT];
static sVTerm *activeVT = NULL;

bool vterm_initAll(tServ *ids) {
	char name[MAX_NAME_LEN + 1];
	u32 i;

	for(i = 0; i < VTERM_COUNT; i++) {
		vterms[i].index = i;
		vterms[i].sid = ids[i];
		sprintf(name,"vterm%d",i);
		memcpy(vterms[i].name,name,MAX_NAME_LEN + 1);
		if(!vterm_init(vterms + i))
			return false;
	}

	startThread(vterm_dateThread);
	return true;
}

sVTerm *vterm_get(u32 index) {
	return vterms + index;
}

static bool vterm_init(sVTerm *vt) {
	tFD vidFd,speakerFd;
	u32 i,len;
	char *ptr,*s;

	/* open video */
	vidFd = open("/drivers/video",IO_WRITE | IO_CONNECT);
	if(vidFd < 0) {
		printe("Unable to open '/services/video'");
		return false;
	}

	/* open speaker */
	speakerFd = open("/services/speaker",IO_WRITE | IO_CONNECT);
	if(speakerFd < 0) {
		printe("Unable to open '/services/speaker'");
		return false;
	}

	/* init state */
	vt->col = 0;
	vt->row = ROWS - 1;
	vt->upStart = 0;
	vt->upLength = 0;
	vt->foreground = WHITE;
	vt->background = BLACK;
	vt->active = false;
	vt->video = vidFd;
	vt->speaker = speakerFd;
	/* start on first line of the last page */
	vt->firstLine = HISTORY_SIZE - ROWS;
	vt->currLine = HISTORY_SIZE - ROWS;
	vt->firstVisLine = HISTORY_SIZE - ROWS;
	/* default behaviour */
	vt->echo = true;
	vt->readLine = true;
	vt->navigation = true;
	vt->keymap = 1;
	vt->escapePos = -1;
	vt->rlStartCol = 0;
	vt->rlBufSize = INITIAL_RLBUF_SIZE;
	vt->rlBufPos = 0;
	vt->rlBuffer = (char*)malloc(vt->rlBufSize * sizeof(char));
	if(vt->rlBuffer == NULL) {
		printe("Unable to allocate memory for vterm-buffer");
		return false;
	}

	vt->inbuf = rb_create(sizeof(char),INPUT_BUF_SIZE,RB_OVERWRITE);
	if(vt->inbuf == NULL) {
		printe("Unable to allocate memory for ring-buffer");
		return false;
	}

	/* fill buffer with spaces to ensure that the cursor is visible (spaces, white on black) */
	ptr = vt->buffer;
	for(i = 0; i < BUFFER_SIZE; i += 4) {
		*ptr++ = 0x20;
		*ptr++ = 0x07;
		*ptr++ = 0x20;
		*ptr++ = 0x07;
	}

	/* build title bar */
	ptr = vt->titleBar;
	s = vt->name;
	for(i = 0; *s; i++) {
		*ptr++ = *s++;
		*ptr++ = TITLE_BAR_COLOR;
	}
	for(; i < COLS; i++) {
		*ptr++ = ' ';
		*ptr++ = TITLE_BAR_COLOR;
	}
	len = strlen(OS_TITLE);
	i = (((COLS * 2) / 2) - (len / 2)) & ~0x1;
	ptr = vt->titleBar;
	memcpy(ptr + i,OS_TITLE,len);
	return true;
}

sVTerm *vterm_getActive(void) {
	return activeVT;
}

void vterm_selectVTerm(u32 index) {
	sVTerm *vt = vterms + index;
	if(activeVT != NULL)
		activeVT->active = false;
	vt->active = true;
	activeVT = vt;

	/* refresh screen and write titlebar */
	vterm_markScrDirty(vt);
	vterm_setCursor(vt);
}

void vterm_destroy(sVTerm *vt) {
	free(vt->rlBuffer);
	close(vt->video);
	close(vt->speaker);
}

void vterm_setCursor(sVTerm *vt) {
	if(vt->active) {
		sIoCtlCursorPos pos;
		pos.col = vt->col;
		pos.row = vt->row;
		ioctl(vt->video,IOCTL_VID_SETCURSOR,(u8*)&pos,sizeof(pos));
	}
}

s32 vterm_ioctl(sVTerm *vt,u32 cmd,void *data,bool *readKb) {
	s32 res = 0;
	UNUSED(data);
	switch(cmd) {
		case IOCTL_VT_EN_ECHO:
			vt->echo = true;
			break;
		case IOCTL_VT_DIS_ECHO:
			vt->echo = false;
			break;
		case IOCTL_VT_EN_RDLINE:
			vt->readLine = true;
			/* reset reading */
			vt->rlBufPos = 0;
			vt->rlStartCol = vt->col;
			break;
		case IOCTL_VT_DIS_RDLINE:
			vt->readLine = false;
			break;
		case IOCTL_VT_EN_RDKB:
			*readKb = true;
			break;
		case IOCTL_VT_DIS_RDKB:
			*readKb = false;
			break;
		case IOCTL_VT_EN_NAVI:
			vt->navigation = true;
			break;
		case IOCTL_VT_DIS_NAVI:
			vt->navigation = false;
			break;
		case IOCTL_VT_BACKUP:
			if(!vt->screenBackup)
				vt->screenBackup = (char*)malloc(ROWS * COLS * 2);
			memcpy(vt->screenBackup,
					vt->buffer + vt->firstVisLine * COLS * 2,
					ROWS * COLS * 2);
			break;
		case IOCTL_VT_RESTORE:
			if(vt->screenBackup) {
				memcpy(vt->buffer + vt->firstVisLine * COLS * 2,
						vt->screenBackup,
						ROWS * COLS * 2);
				free(vt->screenBackup);
				vt->screenBackup = NULL;
				vterm_markScrDirty(vt);
			}
			break;
		default:
			res = ERR_UNSUPPORTED_OPERATION;
			break;
	}
	return res;
}

void vterm_scroll(sVTerm *vt,s16 lines) {
	u16 old = vt->firstVisLine;
	if(lines > 0) {
		/* ensure that we don't scroll above the first line with content */
		vt->firstVisLine = MAX(vt->firstLine,(s16)vt->firstVisLine - lines);
	}
	else {
		/* ensure that we don't scroll behind the last line */
		vt->firstVisLine = MIN(HISTORY_SIZE - ROWS,vt->firstVisLine - lines);
	}

	if(vt->active && old != vt->firstVisLine)
		vterm_markDirty(vt,COLS * 2,(COLS - 1) * ROWS * 2);
}

void vterm_markScrDirty(sVTerm *vt) {
	vterm_markDirty(vt,0,COLS * ROWS * 2);
}

void vterm_markDirty(sVTerm *vt,u16 start,u16 length) {
	if(vt->upLength == 0) {
		vt->upStart = start;
		vt->upLength = length;
	}
	else {
		u16 oldstart = vt->upStart;
		if(start < oldstart)
			vt->upStart = start;
		vt->upLength = MAX(oldstart + vt->upLength,start + length) - vt->upStart;
	}
}

void vterm_update(sVTerm *vt) {
	u32 byteCount;
	if(!vt->active || vt->upLength == 0)
		return;

	/* update title-bar? */
	if(vt->upStart < COLS * 2) {
		locku(&titleBarLock);
		byteCount = MIN(COLS * 2 - vt->upStart,vt->upLength);
		seek(vt->video,vt->upStart,SEEK_SET);
		write(vt->video,vt->titleBar,byteCount);
		vt->upLength -= byteCount;
		vt->upStart = COLS * 2;
		unlocku(&titleBarLock);
	}

	/* refresh the rest */
	byteCount = MIN((ROWS * COLS * 2) - vt->upStart,vt->upLength);
	if(byteCount > 0) {
		seek(vt->video,vt->upStart,SEEK_SET);
		write(vt->video,vt->buffer + (vt->firstVisLine * COLS * 2) + vt->upStart,byteCount);
	}

	/* all synchronized now */
	vt->upStart = 0;
	vt->upLength = 0;
}

static int vterm_dateThread(void) {
	u32 i,j,len;
	char dateStr[30];
	sDate date;
	while(1) {
		/* get date and format it */
		if(getDate(&date) != 0)
			continue;
		len = dateToString(dateStr,30,"%a, %d. %b %Y, %H:%M:%S",&date);

		/* update all vterm-title-bars; use a lock to prevent race-conditions */
		locku(&titleBarLock);
		for(i = 0; i < VTERM_COUNT; i++) {
			char *title = vterms[i].titleBar + (COLS - len) * 2;
			for(j = 0; j < len; j++) {
				*title++ = dateStr[j];
				*title++ = WHITE | (BLUE << 4);
			}
			if(vterms[i].active) {
				seek(vterms[i].video,(COLS - len) * 2,SEEK_SET);
				write(vterms[i].video,vterms[i].titleBar + (COLS - len) * 2,len * 2);
			}
		}
		unlocku(&titleBarLock);

		/* wait a second */
		sleep(1000);
	}
	return EXIT_SUCCESS;
}
