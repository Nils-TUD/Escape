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
#include <esc/driver.h>
#include <stdio.h>
#include <esc/lock.h>
#include <esc/thread.h>
#include <esc/date.h>
#include <string.h>
#include <errors.h>
#include <assert.h>
#include <ringbuffer.h>

#include "vtctrl.h"

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
							"3\x17"

bool vterm_init(sVTerm *vt,sVTSize *vidSize,tFD vidFd,tFD speakerFd) {
	u32 i,len;
	u8 color;
	char *ptr,*s;
	vt->cols = vidSize->width;
	vt->rows = vidSize->height;

	/* by default we have no handlers for that */
	vt->handlerShortcut = NULL;
	vt->setCursor = NULL;

	/* init state */
	vt->col = 0;
	vt->row = vt->rows - 1;
	vt->upStart = 0;
	vt->upLength = 0;
	vt->upScroll = 0;
	vt->foreground = vt->defForeground;
	vt->background = vt->defBackground;
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

	vt->inbufEOF = false;
	vt->inbuf = rb_create(sizeof(char),INPUT_BUF_SIZE,RB_OVERWRITE);
	if(vt->inbuf == NULL) {
		printe("Unable to allocate memory for ring-buffer");
		return false;
	}

	/* fill buffer with spaces to ensure that the cursor is visible (spaces, white on black) */
	ptr = vt->buffer;
	color = (vt->background << 4) | vt->foreground;
	for(i = 0, len = vt->rows * HISTORY_SIZE * vt->cols * 2; i < len; i += 4) {
		*ptr++ = ' ';
		*ptr++ = color;
		*ptr++ = ' ';
		*ptr++ = color;
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

void vterm_destroy(sVTerm *vt) {
	rb_destroy(vt->inbuf);
	free(vt->buffer);
	free(vt->titleBar);
	free(vt->rlBuffer);
}

s32 vterm_ctl(sVTerm *vt,sVTermCfg *cfg,u32 cmd,void *data) {
	s32 res = 0;
	switch(cmd) {
		case MSG_VT_SHELLPID:
			/* do it just once */
			if(vt->shellPid == 0)
				vt->shellPid = *(tPid*)data;
			break;
		case MSG_VT_ENABLE:
			cfg->enabled = true;
			break;
		case MSG_VT_DISABLE:
			cfg->enabled = false;
			break;
		case MSG_VT_EN_ECHO:
			vt->echo = true;
			break;
		case MSG_VT_DIS_ECHO:
			vt->echo = false;
			break;
		case MSG_VT_EN_RDLINE:
			vt->readLine = true;
			/* reset reading */
			vt->rlBufPos = 0;
			vt->rlStartCol = vt->col;
			break;
		case MSG_VT_DIS_RDLINE:
			vt->readLine = false;
			break;
		case MSG_VT_EN_RDKB:
			cfg->readKb = true;
			break;
		case MSG_VT_DIS_RDKB:
			cfg->readKb = false;
			break;
		case MSG_VT_EN_NAVI:
			vt->navigation = true;
			break;
		case MSG_VT_DIS_NAVI:
			vt->navigation = false;
			break;
		case MSG_VT_BACKUP:
			if(!vt->screenBackup)
				vt->screenBackup = (char*)malloc(vt->rows * vt->cols * 2);
			memcpy(vt->screenBackup,
					vt->buffer + vt->firstVisLine * vt->cols * 2,
					vt->rows * vt->cols * 2);
			vt->backupCol = vt->col;
			vt->backupRow = vt->row;
			break;
		case MSG_VT_RESTORE:
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
		case MSG_VT_GETSIZE: {
			sVTSize *size = (sVTSize*)data;
			size->width = vt->cols;
			/* one line for the title */
			size->height = vt->rows - 1;
			res = sizeof(sVTSize);
		}
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
		vt->upScroll -= lines;
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
