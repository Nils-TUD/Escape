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

static void vt_doUpdate(sVTerm *vt);
static void vt_setCursor(sVTerm *vt);

static sScreenMode *modes = NULL;
static size_t modeCount = 0;
static sScreenMode *curMode = NULL;
static char *scrShm = NULL;

bool vt_init(int id,sVTerm *vterm,const char *name,uint cols,uint rows) {
	sScreenMode mode;
	int res;

	/* connect to ui-manager */
	vterm->uimng = open("/dev/uim-ctrl",IO_MSGS);
	if(vterm->uimng < 0)
		error("Unable to open '/dev/uim-ctrl'");
	vterm->uimngid = uimng_getId(vterm->uimng);

	/* get all modes */
	modeCount = screen_getModeCount(vterm->uimng);
	/* TODO actually, we need to filter out the only-gui-modes here */
	modes = (sScreenMode*)malloc(modeCount * sizeof(sScreenMode));
	if(!modes)
		error("Unable to create modes array");
	if((res = screen_getModes(vterm->uimng,modes,modeCount)) < 0)
		error("Unable to get modes");

	/* find a suitable mode */
	res = screen_findTextMode(vterm->uimng,cols,rows,&mode);
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
	return true;
}

void vt_update(sVTerm *vt) {
	locku(&vt->lock);
	vt_doUpdate(vt);
	unlocku(&vt->lock);
}

static void vt_doUpdate(sVTerm *vt) {
	/* if we should scroll, mark the whole screen (without title) as dirty */
	if(vt->upScroll != 0) {
		vt->upCol = 0;
		vt->upRow = MIN(vt->upRow,0);
		vt->upHeight = vt->rows - vt->upRow;
		vt->upWidth = vt->cols;
	}

	if(vt->upWidth > 0) {
		/* update content */
		size_t offset = vt->upRow * vt->cols * 2 + vt->upCol * 2;
		char *startPos = vt->buffer + (vt->firstVisLine * vt->cols * 2) + offset;
		if(vt->upWidth == vt->cols)
			memcpy(scrShm + offset,startPos,vt->upWidth * vt->upHeight * 2);
		else {
			for(size_t i = 0; i < vt->upHeight; ++i) {
				memcpy(scrShm + offset + i * vt->cols * 2,
					startPos + i * vt->cols * 2,vt->upWidth * 2);
			}
		}

		if(screen_update(vt->uimng,vt->upCol,vt->upRow,vt->upWidth,vt->upHeight) < 0)
			printe("Unable to update screen");
	}
	vt_setCursor(vt);

	/* all synchronized now */
	vt->upCol = vt->cols;
	vt->upRow = vt->rows;
	vt->upWidth = 0;
	vt->upHeight = 0;
	vt->upScroll = 0;
}

sScreenMode *vt_getModes(size_t *count) {
	*count = modeCount;
	return modes;
}

static int vt_doSetMode(sVTerm *vt,const char *shmname,sScreenMode *mode) {
	int res = screen_createShm(mode,&scrShm,shmname,VID_MODE_TYPE_TUI,0644);
	if(res < 0)
		return res;
	res = screen_setMode(vt->uimng,VID_MODE_TYPE_TUI,mode->id,shmname,true);
	if(res < 0) {
		munmap(scrShm);
		shm_unlink(shmname);
		return res;
	}
	return 0;
}

int vt_setVideoMode(sVTerm *vt,int mode) {
	ssize_t res;
	for(size_t i = 0; i < modeCount; i++) {
		if(modes[i].id == mode) {
			/* destroy current stuff */
			if(scrShm) {
				munmap(scrShm);
				shm_unlink(vt->name);
			}

			/* try to set new mode */
			res = vt_doSetMode(vt,vt->name,modes + i);
			if(res < 0)
				return res;

			/* resize vterm if necessary */
			if(vt->cols != modes[i].cols || vt->rows != modes[i].rows) {
				if(!vtctrl_resize(vt,modes[i].cols,modes[i].rows)) {
					res = vt_doSetMode(vt,vt->name,curMode);
					if(res < 0)
						error("Unable to restore old mode");
					return -ENOMEM;
				}
			}
			curMode = modes + i;
			return 0;
		}
	}
	return -EINVAL;
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
