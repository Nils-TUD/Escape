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
#include <esc/driver/video.h>
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

static int vt_findMode(sVTermCfg *cfg,uint cols,uint rows,sVTMode *mode);
static void vt_doUpdate(sVTerm *vt);
static void vt_setCursor(sVTerm *vt);
static int vt_dateThread(void *arg);

static sVTermCfg *config;

bool vt_init(int id,sVTerm *vterm,const char *name,sVTermCfg *cfg,uint cols,uint rows) {
	int speakerFd;
	sVTMode mode;
	int res;

	/* find a suitable mode */
	config = cfg;
	res = vt_findMode(cfg,cols,rows,&mode);
	if(res < 0) {
		fprintf(stderr,"Unable to find a suitable mode: %s\n",strerror(-res));
		return false;
	}

	/* open speaker */
	speakerFd = open("/dev/speaker",IO_MSGS);
	/* ignore errors here. in this case we simply don't use it */

	vterm->index = 0;
	vterm->sid = id;
	vterm->defForeground = LIGHTGRAY;
	vterm->defBackground = BLACK;
	vterm->setCursor = vt_setCursor;
	memcpy(vterm->name,name,MAX_VT_NAME_LEN + 1);
	if(!vtctrl_init(vterm,mode.width,mode.height,mode.id,cfg->devFds[mode.device],speakerFd))
		return false;
	vterm->active = true;

	/* set video mode */
	res = video_setMode(cfg->devFds[mode.device],mode.id);
	if(res < 0)
		fprintf(stderr,"Unable to set mode: %s\n",strerror(-res));

	if(startthread(vt_dateThread,vterm) < 0)
		error("Unable to start date-thread");
	return true;
}

void vt_enable(sVTerm *vt,bool setMode) {
	locku(&vt->lock);
	if(setMode)
		video_setMode(vt->video,video_getMode(vt->video));
	/* ensure that we redraw everything and re-set the cursor */
	vt->lastCol = vt->lastRow = -1;
	vtctrl_markScrDirty(vt);
	unlocku(&vt->lock);
}

void vt_update(sVTerm *vt) {
	if(!vt->active)
		return;

	locku(&vt->lock);
	vt_doUpdate(vt);
	unlocku(&vt->lock);
}

static int vt_findMode(sVTermCfg *cfg,uint cols,uint rows,sVTMode *mode) {
	size_t i,count,bestmode;
	uint bestdiff = UINT_MAX;
	sVTMode *modes;

	/* get all modes */
	vtctrl_getModes(cfg,0,&count,true);
	modes = vtctrl_getModes(cfg,count,&count,true);
	if(!modes)
		return -ENOENT;

	/* search for the best matching mode */
	bestmode = count;
	for(i = 0; i < count; i++) {
		uint pixdiff = ABS((int)(modes[i].width * modes[i].height) - (int)(cols * rows));
		if(pixdiff < bestdiff) {
			bestmode = i;
			bestdiff = pixdiff;
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
		if(vt->upStart < vt->cols * 2) {
			byteCount = MIN(vt->cols * 2 - vt->upStart,vt->upLength);
			if(seek(vt->video,vt->upStart,SEEK_SET) < 0)
				printe("[VTERM] Unable to seek in video-device to %d",vt->upStart);
			if(write(vt->video,vt->titleBar,byteCount) < 0)
				printe("[VTERM] Unable to write to video-device");
			vt->upLength -= byteCount;
			vt->upStart = vt->cols * 2;
		}

		/* refresh the rest */
		byteCount = MIN((vt->rows * vt->cols * 2) - vt->upStart,vt->upLength);
		if(byteCount > 0) {
			char *startPos = vt->buffer + (vt->firstVisLine * vt->cols * 2) + vt->upStart;
			if(seek(vt->video,vt->upStart,SEEK_SET) < 0)
				printe("[VTERM] Unable to seek in video-device to %d",vt->upStart);
			if(write(vt->video,startPos,byteCount) < 0)
				printe("[VTERM] Unable to write to video-device");
		}
	}
	vt_setCursor(vt);

	/* all synchronized now */
	vt->upStart = 0;
	vt->upLength = 0;
	vt->upScroll = 0;
}

static void vt_setCursor(sVTerm *vt) {
	if(vt->active) {
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
			video_setCursor(vt->video,&pos);
			vt->lastCol = pos.col;
			vt->lastRow = pos.row;
		}
	}
}

static int vt_dateThread(A_UNUSED void *arg) {
	size_t j,len;
	sVTerm *vterm = (sVTerm*)arg;
	char dateStr[SSTRLEN("Mon, 14. Jan 2009, 12:13:14") + 1];
	while(1) {
		if(config->enabled) {
			/* get date and format it */
			time_t now = time(NULL);
			struct tm *date = gmtime(&now);
			len = strftime(dateStr,sizeof(dateStr),"%a, %d. %b %Y, %H:%M:%S",date);

			/* update vterm-title-bar */
			char *title;
			locku(&vterm->lock);
			title = vterm->titleBar + (vterm->cols - len) * 2;
			for(j = 0; j < len; j++) {
				*title++ = dateStr[j];
				*title++ = LIGHTGRAY | (BLUE << 4);
			}
			if(seek(vterm->video,(vterm->cols - len) * 2,SEEK_SET) < 0)
				printe("[VTERM] Unable to seek in video-device");
			if(write(vterm->video,vterm->titleBar + (vterm->cols - len) * 2,len * 2) < 0)
				printe("[VTERM] Unable to write to video-device");
			unlocku(&vterm->lock);
		}

		/* wait a second */
		sleep(1000);
	}
	return EXIT_SUCCESS;
}
