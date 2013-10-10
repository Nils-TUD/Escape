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
#include <esc/driver/screen.h>
#include <esc/driver/uimng.h>
#include <esc/thread.h>
#include <esc/io.h>
#include <esc/keycodes.h>
#include <esc/driver.h>
#include <esc/proc.h>
#include <esc/conf.h>
#include <esc/mem.h>
#include <esc/messages.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>

#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include "vterm.h"

#define ABS(x)			((x) > 0 ? (x) : -(x))

static int vt_findMode(int uimng,uint cols,uint rows,sScreenMode *mode);
static void vt_doUpdate(sVTerm *vt);
static void vt_setCursor(sVTerm *vt);
static int vt_dateThread(void *arg);

static int scrMode = -1;
static char *scrShm = NULL;

bool vt_init(int id,sVTerm *vterm,const char *name,uint cols,uint rows) {
	sScreenMode mode;
	int res;

	/* connect to ui-manager */
	vterm->uimng = open("/dev/uim-ctrl",IO_MSGS);
	if(vterm->uimng < 0)
		error("Unable to open '/dev/uim-ctrl'");
	vterm->uimngid = uimng_getId(vterm->uimng);

	/* find a suitable mode */
	res = vt_findMode(vterm->uimng,cols,rows,&mode);
	if(res < 0) {
		fprintf(stderr,"Unable to find a suitable mode: %s\n",strerror(-res));
		return false;
	}

	/* open speaker */
	vterm->speaker = open("/dev/speaker",IO_MSGS);
	/* ignore errors here. in this case we simply don't use it */

	vterm->index = 0;
	vterm->sid = id;
	vterm->defForeground = LIGHTGRAY;
	vterm->defBackground = BLACK;
	vterm->setCursor = vt_setCursor;
	memcpy(vterm->name,name,MAX_VT_NAME_LEN + 1);
	if(!vtctrl_init(vterm,&mode))
		return false;

	/* set video mode */
	res = vt_setVideoMode(vterm,mode.id);
	if(res < 0)
		fprintf(stderr,"Unable to set mode: %s\n",strerror(-res));

	/*if(startthread(vt_dateThread,vterm) < 0)
		error("Unable to start date-thread");*/
	return true;
}

void vt_update(sVTerm *vt) {
	locku(&vt->lock);
	vt_doUpdate(vt);
	unlocku(&vt->lock);
}

static int vt_findMode(int uimng,uint cols,uint rows,sScreenMode *mode) {
	ssize_t i,count,res;
	size_t bestmode;
	uint bestdiff = UINT_MAX;
	sScreenMode *modes;

	/* get all modes */
	count = screen_getModeCount(uimng);
	if(count < 0)
		return count;
	modes = (sScreenMode*)malloc(count * sizeof(sScreenMode));
	if(!modes)
		return -ENOMEM;
	if((res = screen_getModes(uimng,modes,count)) < 0) {
		free(modes);
		return res;
	}

	/* search for the best matching mode */
	bestmode = count;
	for(i = 0; i < count; i++) {
		if(modes[i].type & VID_MODE_TYPE_TUI) {
			uint pixdiff = ABS((int)(modes[i].rows * modes[i].cols) - (int)(cols * rows));
			if(pixdiff < bestdiff) {
				bestmode = i;
				bestdiff = pixdiff;
			}
		}
	}
	memcpy(mode,modes + bestmode,sizeof(sScreenMode));
	free(modes);
	return 0;
}

static void vt_doUpdate(sVTerm *vt) {
	/* if we should scroll, mark the whole screen (without title) as dirty */
	if(vt->upScroll != 0) {
		vt->upCol = 0;
		vt->upRow = MIN(vt->upRow,1);
		vt->upHeight = vt->rows - vt->upRow;
		vt->upWidth = vt->cols;
	}

	if(vt->upWidth > 0) {
		/* update title-bar? */
		uint upRow = vt->upRow;
		size_t upHeight = vt->upHeight;
		if(upRow == 0) {
			memcpy(scrShm + vt->upCol * 2,vt->titleBar,vt->upWidth * 2);
			upRow++;
			upHeight--;
		}

		/* update content? */
		if(upHeight > 0) {
			size_t offset = upRow * vt->cols * 2 + vt->upCol * 2;
			char *startPos = vt->buffer + (vt->firstVisLine * vt->cols * 2) + offset;
			if(vt->upWidth == vt->cols)
				memcpy(scrShm + offset,startPos,vt->upWidth * upHeight * 2);
			else {
				for(size_t i = 0; i < upHeight; ++i) {
					memcpy(scrShm + offset + i * vt->cols * 2,
						startPos + i * vt->cols * 2,vt->upWidth * 2);
				}
			}
		}

		if(screen_update(vt->uimng,vt->upCol,vt->upRow,vt->upWidth,vt->upHeight) < 0)
			printe("[VTERM] Unable to update screen");
	}
	vt_setCursor(vt);

	/* all synchronized now */
	vt->upCol = vt->cols;
	vt->upRow = vt->rows;
	vt->upWidth = 0;
	vt->upHeight = 0;
	vt->upScroll = 0;
}

sScreenMode *vt_getModes(sVTerm *vt,size_t n,size_t *count) {
	ssize_t res;
	if(n == 0) {
		*count = screen_getModeCount(vt->uimng);
		return NULL;
	}

	/* TODO actually, we need to filter out the only-gui-modes here */
	sScreenMode *modes = (sScreenMode*)malloc(n * sizeof(sScreenMode));
	if(!modes)
		return NULL;
	if((res = screen_getModes(vt->uimng,modes,n)) < 0) {
		free(modes);
		*count = res;
		return NULL;
	}
	*count = n;
	return modes;
}

static int vt_doSetMode(sVTerm *vt,const char *shmname,uint cols,uint rows,int mid) {
	int fd = shm_open(shmname,IO_READ | IO_WRITE | IO_CREATE,0777);
	if(fd < 0)
		return fd;
	scrShm = mmap(NULL,cols * rows * 2,0,PROT_READ | PROT_WRITE,
		MAP_SHARED,fd,0);
	close(fd);
	if(!scrShm)
		return -ENOMEM;
	int res = screen_setMode(vt->uimng,VID_MODE_TYPE_TUI,mid,shmname,true);
	if(res < 0) {
		munmap(scrShm);
		shm_unlink(shmname);
		return res;
	}
	return 0;
}

int vt_setVideoMode(sVTerm *vt,int mode) {
	/* get all modes */
	size_t i,count;
	vt_getModes(vt,0,&count);
	sScreenMode *modes = vt_getModes(vt,count,&count);
	if(!modes)
		return -ENOMEM;

	ssize_t res;
	for(i = 0; i < count; i++) {
		if(modes[i].id == mode) {
			/* destroy current stuff */
			if(scrShm) {
				munmap(scrShm);
				shm_unlink(vt->name);
			}

			/* try to set new mode */
			res = vt_doSetMode(vt,vt->name,modes[i].cols,modes[i].rows,modes[i].id);
			if(res < 0)
				goto error;

			/* resize vterm if necessary */
			if(vt->cols != modes[i].cols || vt->rows != modes[i].rows) {
				if(!vtctrl_resize(vt,modes[i].cols,modes[i].rows)) {
					res = vt_doSetMode(vt,vt->name,vt->cols,vt->rows,scrMode);
					if(res < 0)
						error("Unable to restore old mode");
					res = -ENOMEM;
					goto error;
				}
			}
			scrMode = mode;
			free(modes);
			return 0;
		}
	}
	res = -EINVAL;

error:
	free(modes);
	return res;
}

static void vt_setCursor(sVTerm *vt) {
	gpos_t x,y;
	if(vt->upScroll != 0 || vt->col != vt->lastCol || vt->row != vt->lastRow) {
		/* draw no cursor if it's not visible by setting it to an invalid location */
		if(vt->firstVisLine + vt->rows <= vt->currLine + vt->row) {
			x = vt->cols;
			y = vt->rows;
		}
		else {
			x = vt->col;
			y = vt->row;
		}
		screen_setCursor(vt->uimng,x,y,CURSOR_DEFAULT);
		vt->lastCol = x;
		vt->lastRow = y;
	}
}

static int vt_dateThread(A_UNUSED void *arg) {
	size_t j,len;
	sVTerm *vt = (sVTerm*)arg;
	char dateStr[SSTRLEN("Mon, 14. Jan 2009, 12:13:14") + 1];
	while(1) {
		/* get date and format it */
		time_t now = time(NULL);
		struct tm *date = gmtime(&now);
		len = strftime(dateStr,sizeof(dateStr),"%a, %d. %b %Y, %H:%M:%S",date);

		/* update vterm-title-bar */
		char *title;
		locku(&vt->lock);
		title = vt->titleBar + (vt->cols - len) * 2;
		for(j = 0; j < len; j++) {
			*title++ = dateStr[j];
			*title++ = LIGHTGRAY | (BLUE << 4);
		}
		memcpy(scrShm + (vt->cols - len) * 2,vt->titleBar + (vt->cols - len) * 2,len * 2);
		if(screen_update(vt->uimng,vt->cols - len,0,len * 2,1) < 0)
			printe("[VTERM] Unable to update screen");
		unlocku(&vt->lock);

		/* wait a second */
		sleep(1000);
	}
	return EXIT_SUCCESS;
}
