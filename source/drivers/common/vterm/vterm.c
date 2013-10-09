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
#include <esc/driver/uimng.h>
#include <esc/thread.h>
#include <esc/io.h>
#include <esc/keycodes.h>
#include <esc/driver.h>
#include <esc/proc.h>
#include <esc/conf.h>
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

static int vt_findMode(int uimng,uint cols,uint rows,sVTMode *mode);
static void vt_doUpdate(sVTerm *vt);
static void vt_setCursor(sVTerm *vt);
static int vt_dateThread(void *arg);

bool vt_init(int id,sVTerm *vterm,const char *name,uint cols,uint rows) {
	sVTMode mode;
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
	res = vtctrl_setVideoMode(vterm,mode.id);
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

static int vt_findMode(int uimng,uint cols,uint rows,sVTMode *mode) {
	ssize_t i,count,res;
	size_t bestmode;
	uint bestdiff = UINT_MAX;
	sVTMode *modes;

	/* get all modes */
	count = uimng_getModeCount(uimng);
	if(count < 0)
		return count;
	modes = (sVTMode*)malloc(count * sizeof(sVTMode));
	if(!modes)
		return -ENOMEM;
	if((res = uimng_getModes(uimng,modes,count)) < 0) {
		free(modes);
		return res;
	}

	/* search for the best matching mode */
	bestmode = count;
	for(i = 0; i < count; i++) {
		if(modes[i].type & VID_MODE_TYPE_TUI) {
			uint pixdiff = ABS((int)(modes[i].width * modes[i].height) - (int)(cols * rows));
			if(pixdiff < bestdiff) {
				bestmode = i;
				bestdiff = pixdiff;
			}
		}
	}
	memcpy(mode,modes + bestmode,sizeof(sVTMode));
	free(modes);
	return 0;
}

static void vt_doUpdate(sVTerm *vt) {
	size_t byteCount;
	/* if we should scroll, mark the whole screen (without title) as dirty */
	if(vt->upScroll != 0) {
		vt->upStart = MIN(vt->upStart,vt->cols * 2);
		vt->upLength = vt->cols * vt->rows * 2 - vt->upStart;
	}

	if(vt->upLength > 0) {
		/* update title-bar? */
		uint start = vt->upStart;
		uint length = vt->upLength;
		if(start < vt->cols * 2) {
			byteCount = MIN(vt->cols * 2 - start,length);
			memcpy(vt->scrShm + start,vt->titleBar,byteCount);
			length -= byteCount;
			start = vt->cols * 2;
		}

		byteCount = MIN((vt->rows * vt->cols * 2) - start,length);
		if(byteCount > 0) {
			char *startPos = vt->buffer + (vt->firstVisLine * vt->cols * 2) + start;
			memcpy(vt->scrShm + start,startPos,byteCount);
		}

		if(uimng_update(vt->uimng,vt->upStart,vt->upLength) < 0)
			printe("[VTERM] Unable to update screen");
	}
	vt_setCursor(vt);

	/* all synchronized now */
	vt->upStart = 0;
	vt->upLength = 0;
	vt->upScroll = 0;
}

static void vt_setCursor(sVTerm *vt) {
	sVTPos pos;
	if(vt->upScroll != 0 || vt->col != vt->lastCol || vt->row != vt->lastRow) {
		/* draw no cursor if it's not visible by setting it to an invalid location */
		if(vt->firstVisLine + vt->rows <= vt->currLine + vt->row) {
			pos.col = vt->cols;
			pos.row = vt->rows;
		}
		else {
			pos.col = vt->col;
			pos.row = vt->row;
		}
		uimng_setCursor(vt->uimng,&pos);
		vt->lastCol = pos.col;
		vt->lastRow = pos.row;
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
		memcpy(vt->scrShm + (vt->cols - len) * 2,vt->titleBar + (vt->cols - len) * 2,len * 2);
		if(uimng_update(vt->uimng,(vt->cols - len) * 2,len * 2) < 0)
			printe("[VTERM] Unable to update screen");
		unlocku(&vt->lock);

		/* wait a second */
		sleep(1000);
	}
	return EXIT_SUCCESS;
}
