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
#include <assert.h>
#include <ringbuffer.h>

#include "vterm.h"

#define VIDEO_DRIVER		"/drivers/video"

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
							"2\x17"

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
static int vterm_dateThread(int argc,char *argv[]);

/* vterms */
static tULock titleBarLock;
static sVTerm vterms[VTERM_COUNT];
static sVTerm *activeVT = NULL;

bool vterm_initAll(tServ *ids) {
	char name[MAX_VT_NAME_LEN + 1];
	u32 i;

	for(i = 0; i < VTERM_COUNT; i++) {
		vterms[i].index = i;
		vterms[i].sid = ids[i];
		sprintf(name,"vterm%d",i);
		memcpy(vterms[i].name,name,MAX_VT_NAME_LEN + 1);
		if(!vterm_init(vterms + i))
			return false;
	}

	startThread(vterm_dateThread,NULL);
	return true;
}

sVTerm *vterm_get(u32 index) {
	return vterms + index;
}

static bool vterm_init(sVTerm *vt) {
	tFD vidFd,speakerFd;
	u32 i,len;
	char *ptr,*s;
	sIoCtlSize vidSize;

	/* open video */
	vidFd = open(VIDEO_DRIVER,IO_WRITE);
	if(vidFd < 0) {
		printe("Unable to open '%s'",VIDEO_DRIVER);
		return false;
	}

	/* open speaker */
	speakerFd = open("/services/speaker",IO_WRITE);
	if(speakerFd < 0) {
		printe("Unable to open '/services/speaker'");
		return false;
	}

	/* request screensize from video-driver */
	if(ioctl(vidFd,IOCTL_VID_GETSIZE,&vidSize,sizeof(sIoCtlSize)) < 0) {
		printe("Getting screensize failed");
		return false;
	}
	vt->cols = vidSize.width;
	vt->rows = vidSize.height;

	/* init state */
	vt->col = 0;
	vt->row = vt->rows - 1;
	vt->upStart = 0;
	vt->upLength = 0;
	vt->foreground = WHITE;
	vt->background = BLACK;
	vt->active = false;
	vt->video = vidFd;
	vt->speaker = speakerFd;
	/* start on first line of the last page */
	vt->firstLine = HISTORY_SIZE * vt->rows - vt->rows;
	vt->currLine = HISTORY_SIZE * vt->rows - vt->rows;
	vt->firstVisLine = HISTORY_SIZE * vt->rows - vt->rows;
	/* default behaviour */
	vt->echo = true;
	vt->readLine = true;
	vt->navigation = true;
	vt->printToRL = false;
	vt->keymap = 1;
	vt->escapePos = -1;
	vt->rlStartCol = 0;
	vt->shellPid = 0;
	vt->buffer = (char*)malloc(vt->rows * HISTORY_SIZE * vt->cols * 2);
	if(vt->buffer == NULL) {
		printe("Unable to allocate mem for vterm-buffer");
		return false;
	}
	vt->titleBar = (char*)malloc(vt->cols * 2);
	if(vt->titleBar == NULL) {
		printe("Unable to allocate mem for vterm-titlebar");
		return false;
	}
	vt->rlBufSize = INITIAL_RLBUF_SIZE;
	vt->rlBufPos = 0;
	vt->rlBuffer = (char*)malloc(vt->rlBufSize * sizeof(char));
	if(vt->rlBuffer == NULL) {
		printe("Unable to allocate memory for readline-buffer");
		return false;
	}

	vt->inbuf = rb_create(sizeof(char),INPUT_BUF_SIZE,RB_OVERWRITE);
	if(vt->inbuf == NULL) {
		printe("Unable to allocate memory for ring-buffer");
		return false;
	}

	/* fill buffer with spaces to ensure that the cursor is visible (spaces, white on black) */
	ptr = vt->buffer;
	for(i = 0, len = vt->rows * HISTORY_SIZE * vt->cols * 2; i < len; i += 4) {
		*ptr++ = ' ';
		*ptr++ = 0x07;
		*ptr++ = ' ';
		*ptr++ = 0x07;
	}

	/* build title bar */
	ptr = vt->titleBar;
	s = vt->name;
	for(i = 0; *s; i++) {
		*ptr++ = *s++;
		*ptr++ = TITLE_BAR_COLOR;
	}
	for(; i < vt->cols; i++) {
		*ptr++ = ' ';
		*ptr++ = TITLE_BAR_COLOR;
	}
	len = strlen(OS_TITLE);
	i = (((vt->cols * 2) / 2) - (len / 2)) & ~0x1;
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
		sIoCtlPos pos;
		pos.col = vt->col;
		pos.row = vt->row;
		ioctl(vt->video,IOCTL_VID_SETCURSOR,(u8*)&pos,sizeof(pos));
	}
}

s32 vterm_ioctl(sVTerm *vt,u32 cmd,void *data,bool *readKb) {
	s32 res = 0;
	UNUSED(data);
	switch(cmd) {
		case IOCTL_VT_SHELLPID:
			/* do it just once */
			if(vt->shellPid == 0)
				vt->shellPid = *(tPid*)data;
			break;
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
				vt->screenBackup = (char*)malloc(vt->rows * vt->cols * 2);
			memcpy(vt->screenBackup,
					vt->buffer + vt->firstVisLine * vt->cols * 2,
					vt->rows * vt->cols * 2);
			vt->backupCol = vt->col;
			vt->backupRow = vt->row;
			break;
		case IOCTL_VT_RESTORE:
			if(vt->screenBackup) {
				memcpy(vt->buffer + vt->firstVisLine * vt->cols * 2,
						vt->screenBackup,
						vt->rows * vt->cols * 2);
				free(vt->screenBackup);
				vt->screenBackup = NULL;
				vt->col = vt->backupCol;
				vt->row = vt->backupRow;
				vterm_markScrDirty(vt);
			}
			break;
		case IOCTL_VT_GETSIZE: {
			sIoCtlSize *size = (sIoCtlSize*)data;
			size->width = vt->cols;
			/* one line for the title */
			size->height = vt->rows - 1;
			res = sizeof(sIoCtlSize);
		}
		break;
		default:
			res = ERR_UNSUPPORTED_OP;
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
		vt->firstVisLine = MIN(HISTORY_SIZE * vt->rows - vt->rows,vt->firstVisLine - lines);
	}

	if(vt->active && old != vt->firstVisLine)
		vterm_markDirty(vt,vt->cols * 2,(vt->cols - 1) * vt->rows * 2);
}

void vterm_markScrDirty(sVTerm *vt) {
	vterm_markDirty(vt,0,vt->cols * vt->rows * 2);
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
	if(!vt->active)
		return;

	if(vt->upLength > 0) {
		/* update title-bar? */
		if(vt->upStart < vt->cols * 2) {
			locku(&titleBarLock);
			byteCount = MIN(vt->cols * 2 - vt->upStart,vt->upLength);
			seek(vt->video,vt->upStart,SEEK_SET);
			write(vt->video,vt->titleBar,byteCount);
			vt->upLength -= byteCount;
			vt->upStart = vt->cols * 2;
			unlocku(&titleBarLock);
		}

		/* refresh the rest */
		byteCount = MIN((vt->rows * vt->cols * 2) - vt->upStart,vt->upLength);
		if(byteCount > 0) {
			seek(vt->video,vt->upStart,SEEK_SET);
			write(vt->video,vt->buffer + (vt->firstVisLine * vt->cols * 2) + vt->upStart,byteCount);
		}
	}
	vterm_setCursor(vt);

	/* all synchronized now */
	vt->upStart = 0;
	vt->upLength = 0;
}

static int vterm_dateThread(int argc,char *argv[]) {
	u32 i,j,len;
	char dateStr[SSTRLEN("Mon, 14. Jan 2009, 12:13:14") + 1];
	sDate date;
	UNUSED(argc);
	UNUSED(argv);
	while(1) {
		/* get date and format it */
		if(getDate(&date) != 0)
			continue;
		len = dateToString(dateStr,30,"%a, %d. %b %Y, %H:%M:%S",&date);

		/* update all vterm-title-bars; use a lock to prevent race-conditions */
		locku(&titleBarLock);
		for(i = 0; i < VTERM_COUNT; i++) {
			char *title = vterms[i].titleBar + (vterms[i].cols - len) * 2;
			for(j = 0; j < len; j++) {
				*title++ = dateStr[j];
				*title++ = WHITE | (BLUE << 4);
			}
			if(vterms[i].active) {
				seek(vterms[i].video,(vterms[i].cols - len) * 2,SEEK_SET);
				write(vterms[i].video,vterms[i].titleBar + (vterms[i].cols - len) * 2,len * 2);
			}
		}
		unlocku(&titleBarLock);

		/* wait a second */
		sleep(1000);
	}
	return EXIT_SUCCESS;
}
