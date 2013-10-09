/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/proc.h>
#include <esc/keycodes.h>
#include <esc/driver.h>
#include <esc/thread.h>
#include <esc/messages.h>
#include <esc/ringbuffer.h>
#include <esc/driver/video.h>
#include <esc/driver/uimng.h>
#include <esc/conf.h>
#include <esc/mem.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <vterm/vtctrl.h>

/* the number of chars to keep in history */
#define INITIAL_RLBUF_SIZE	50

#define TITLE_BAR_COLOR		0x17
#define OS_TITLE			"Escape v"ESCAPE_VERSION

static void vtctrl_buildTitle(sVTerm *vt);

bool vtctrl_init(sVTerm *vt,sVTMode *mode) {
	size_t i,len;
	uchar color;
	char *ptr;
	vt->cols = mode->width;
	vt->rows = mode->height;
	vt->scrShm = NULL;
	vt->scrMode = -1;

	/* by default we have no handlers for that */
	vt->setCursor = NULL;

	/* init state */
	if(crtlocku(&vt->lock) < 0) {
		printe("Unable to create vterm lock");
		return false;
	}
	vt->col = 0;
	vt->row = vt->rows - 1;
	vt->lastCol = 0;
	vt->lastRow = 0;
	vt->upStart = 0;
	vt->upLength = 0;
	vt->upScroll = 0;
	vt->foreground = vt->defForeground;
	vt->background = vt->defBackground;
	/* start on first line of the last page */
	vt->firstLine = HISTORY_SIZE * vt->rows - vt->rows;
	vt->currLine = HISTORY_SIZE * vt->rows - vt->rows;
	vt->firstVisLine = HISTORY_SIZE * vt->rows - vt->rows;
	/* default behaviour */
	vt->echo = true;
	vt->readLine = true;
	vt->navigation = true;
	vt->printToRL = false;
	vt->printToCom1 = sysconf(CONF_LOG);
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
	for(i = 0, len = vt->rows * HISTORY_SIZE * vt->cols * 2; i < len; i += 2) {
		*ptr++ = ' ';
		*ptr++ = color;
	}

	vtctrl_buildTitle(vt);
	return true;
}

void vtctrl_destroy(sVTerm *vt) {
	rb_destroy(vt->inbuf);
	free(vt->buffer);
	free(vt->titleBar);
	free(vt->rlBuffer);
}

sVTMode *vtctrl_getModes(sVTerm *vt,size_t n,size_t *count) {
	ssize_t res;
	if(n == 0) {
		*count = uimng_getModeCount(vt->uimng);
		return NULL;
	}

	sVTMode *modes = (sVTMode*)malloc(n * sizeof(sVTMode));
	if(!modes)
		return NULL;
	if((res = uimng_getModes(vt->uimng,modes,n)) < 0) {
		free(modes);
		*count = res;
		return NULL;
	}
	*count = n;
	return modes;
}

static int vtctrl_doSetMode(sVTerm *vt,const char *shmname,uint cols,uint rows,int mid) {
	int fd = shm_open(shmname,IO_READ | IO_WRITE | IO_CREATE,0777);
	if(fd < 0)
		return fd;
	vt->scrShm = mmap(NULL,cols * rows * 2,0,PROT_READ | PROT_WRITE,
		MAP_SHARED,fd,0);
	close(fd);
	if(!vt->scrShm)
		return -ENOMEM;
	int res = uimng_setMode(vt->uimng,mid,shmname);
	if(res < 0) {
		munmap(vt->scrShm);
		shm_unlink(shmname);
		return res;
	}
	return 0;
}

int vtctrl_setVideoMode(sVTerm *vt,int mode) {
	/* get all modes */
	size_t i,count;
	vtctrl_getModes(vt,0,&count);
	sVTMode *modes = vtctrl_getModes(vt,count,&count);
	if(!modes)
		return -ENOMEM;

	ssize_t res;
	for(i = 0; i < count; i++) {
		if(modes[i].id == mode) {
			/* destroy current stuff */
			if(vt->scrShm) {
				munmap(vt->scrShm);
				shm_unlink(vt->name);
			}

			/* try to set new mode */
			res = vtctrl_doSetMode(vt,vt->name,modes[i].width,modes[i].height,modes[i].id);
			if(res < 0)
				goto error;

			/* resize vterm if necessary */
			if(vt->cols != modes[i].width || vt->rows != modes[i].height) {
				if(!vtctrl_resize(vt,modes[i].width,modes[i].height)) {
					res = vtctrl_doSetMode(vt,vt->name,vt->cols,vt->rows,vt->scrMode);
					if(res < 0)
						error("Unable to restore old mode");
					res = -ENOMEM;
					goto error;
				}
			}
			vt->scrMode = mode;
			free(modes);
			return 0;
		}
	}
	res = -EINVAL;

error:
	free(modes);
	return res;
}

bool vtctrl_resize(sVTerm *vt,size_t cols,size_t rows) {
	bool res = false;
	locku(&vt->lock);
	if(vt->cols != cols || vt->rows != rows) {
		size_t c,r,color;
		size_t ccols = MIN(cols,vt->cols);
		char *buf,*oldBuf,*old = vt->buffer;
		char *oldTitle = vt->titleBar;
		vt->buffer = (char*)malloc(rows * HISTORY_SIZE * cols * 2);
		if(vt->buffer == NULL) {
			vt->buffer = old;
			unlocku(&vt->lock);
			return false;
		}
		vt->titleBar = (char*)malloc(cols * 2);
		if(vt->titleBar == NULL) {
			vt->titleBar = oldTitle;
			vt->buffer = old;
			unlocku(&vt->lock);
			return false;
		}

		buf = vt->buffer;
		color = (vt->background << 4) | vt->foreground;
		r = 0;
		if(rows > vt->rows) {
			size_t limit = (rows - vt->rows) * HISTORY_SIZE;
			for(; r < limit; r++) {
				for(c = 0; c < cols; c++) {
					*buf++ = ' ';
					*buf++ = color;
				}
			}
			oldBuf = old;
		}
		else
			oldBuf = old + (vt->rows - rows) * HISTORY_SIZE * vt->cols * 2;
		for(; r < rows * HISTORY_SIZE; r++) {
			memcpy(buf,oldBuf,ccols * 2);
			buf += ccols * 2;
			oldBuf += vt->cols * 2;
			for(c = ccols; c < cols; c++) {
				*buf++ = ' ';
				*buf++ = color;
			}
		}

		if(vt->rows * HISTORY_SIZE - vt->firstLine >= rows * HISTORY_SIZE)
			vt->firstLine = 0;
		else
			vt->firstLine = rows * HISTORY_SIZE - (vt->rows * HISTORY_SIZE - vt->firstLine);
		vt->firstLine += vt->rows - rows;
		if(vt->rows * HISTORY_SIZE - vt->currLine >= rows * HISTORY_SIZE)
			vt->currLine = 0;
		else
			vt->currLine = rows * HISTORY_SIZE - (vt->rows * HISTORY_SIZE - vt->currLine);
		vt->currLine += vt->rows - rows;
		if(vt->rows * HISTORY_SIZE - vt->firstVisLine >= rows * HISTORY_SIZE)
			vt->firstVisLine = 0;
		else
			vt->firstVisLine = rows * HISTORY_SIZE - (vt->rows * HISTORY_SIZE - vt->firstVisLine);
		vt->firstVisLine += vt->rows - rows;

		/* TODO update screenbackup */
		vt->col = MIN(vt->col,cols - 1);
		vt->row = MIN(rows - 1,rows - (vt->rows - vt->row));
		vt->cols = cols;
		vt->rows = rows;
		vt->upStart = 0;
		vt->upLength = 0;
		vtctrl_markScrDirty(vt);
		vtctrl_buildTitle(vt);
		free(old);
		free(oldTitle);
		res = true;
	}
	unlocku(&vt->lock);
	return res;
}

int vtctrl_control(sVTerm *vt,uint cmd,void *data) {
	int res = 0;
	locku(&vt->lock);
	switch(cmd) {
		case MSG_VT_SHELLPID:
			vt->shellPid = *(pid_t*)data;
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
		case MSG_VT_EN_NAVI:
			vt->navigation = true;
			break;
		case MSG_VT_DIS_NAVI:
			vt->navigation = false;
			break;
		case MSG_VT_GETMODE:
			res = vt->scrMode;
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
				vtctrl_markScrDirty(vt);
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
	unlocku(&vt->lock);
	return res;
}

void vtctrl_scroll(sVTerm *vt,int lines) {
	size_t old;
	old = vt->firstVisLine;
	if(lines > 0) {
		/* ensure that we don't scroll above the first line with content */
		vt->firstVisLine = MAX((int)vt->firstLine,(int)vt->firstVisLine - lines);
	}
	else {
		/* ensure that we don't scroll behind the last line */
		vt->firstVisLine = MIN(HISTORY_SIZE * vt->rows - vt->rows,vt->firstVisLine - lines);
	}

	if(old != vt->firstVisLine)
		vt->upScroll -= lines;
}

void vtctrl_markScrDirty(sVTerm *vt) {
	vtctrl_markDirty(vt,0,vt->cols * vt->rows * 2);
}

void vtctrl_markDirty(sVTerm *vt,size_t start,size_t length) {
	if(vt->upLength == 0) {
		vt->upStart = start;
		vt->upLength = length;
	}
	else {
		size_t oldstart = vt->upStart;
		if(start < oldstart)
			vt->upStart = start;
		vt->upLength = MAX(oldstart + vt->upLength,start + length) - vt->upStart;
	}
}

static void vtctrl_buildTitle(sVTerm *vt) {
	size_t i,j,len;
	char *ptr = vt->titleBar;
	char *s = vt->name;
	for(i = 0; *s; i++) {
		*ptr++ = *s++;
		*ptr++ = TITLE_BAR_COLOR;
	}
	for(; i < vt->cols; i++) {
		*ptr++ = ' ';
		*ptr++ = TITLE_BAR_COLOR;
	}
	len = strlen(OS_TITLE);
	i = (((vt->cols * 2) / 2) - len) & ~0x1;
	ptr = vt->titleBar;
	for(j = 0; j < len; j++) {
		ptr[i + j * 2] = OS_TITLE[j];
		ptr[i + j * 2 + 1] = TITLE_BAR_COLOR;
	}
}
