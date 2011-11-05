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

#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include "vterm.h"

#define VGA_DEVICE		"/dev/video"
#define VESA_DEVICE		"/dev/vesatext"

static void vt_doUpdate(sVTerm *vt);
static void vt_setCursor(sVTerm *vt);
static int vt_dateThread(void *arg);

/* vterms */
static tULock vtLock;
static sVTerm vterms[VTERM_COUNT];
static sVTerm *activeVT = NULL;
static sVTermCfg *config;

sVTerm *vt_get(size_t index) {
	return vterms + index;
}

bool vt_initAll(int *ids,sVTermCfg *cfg) {
	int vidFd,speakerFd;
	sVTSize vidSize;
	char name[MAX_VT_NAME_LEN + 1];
	const char *device;
	size_t i;

	config = cfg;

	/* open video-device */
	if(getConf(CONF_BOOT_VIDEOMODE) == CONF_VIDMODE_VESATEXT)
		device = VESA_DEVICE;
	else
		device = VGA_DEVICE;
	vidFd = open(device,IO_READ | IO_WRITE | IO_MSGS);
	if(vidFd < 0) {
		printe("Unable to open '%s'",device);
		return false;
	}

	/* request screensize from video-device */
	if(video_getSize(vidFd,&vidSize) < 0) {
		printe("Getting screensize failed");
		return false;
	}

	/* open speaker */
	speakerFd = open("/dev/speaker",IO_MSGS);
	/* ignore errors here. in this case we simply don't use it */

	for(i = 0; i < VTERM_COUNT; i++) {
		vterms[i].index = i;
		vterms[i].sid = ids[i];
		vterms[i].defForeground = LIGHTGRAY;
		vterms[i].defBackground = BLACK;
		snprintf(name,sizeof(name),"vterm%d",i);
		memcpy(vterms[i].name,name,MAX_VT_NAME_LEN + 1);
		if(!vtctrl_init(vterms + i,&vidSize,vidFd,speakerFd))
			return false;

		vterms[i].setCursor = vt_setCursor;
	}

	if(startThread(vt_dateThread,NULL) < 0)
		error("Unable to start date-thread");
	return true;
}

sVTerm *vt_getActive(void) {
	return activeVT;
}

void vt_selectVTerm(size_t index) {
	locku(&vtLock);
	if(!activeVT || activeVT->index != index) {
		sVTerm *vt = vterms + index;
		if(activeVT != NULL)
			activeVT->active = false;
		vt->active = true;
		/* force cursor-update */
		vt->lastCol = vt->cols;
		activeVT = vt;

		/* refresh screen and write titlebar */
		locku(&vt->lock);
		vtctrl_markScrDirty(vt);
		vt_setCursor(vt);
		vt_doUpdate(vt);
		unlocku(&vt->lock);
	}
	unlocku(&vtLock);
}

void vt_enable(void) {
	sVTerm* vt = vt_getActive();
	if(vt) {
		locku(&vt->lock);
		video_setMode(vt->video);
		vtctrl_markScrDirty(vt);
		unlocku(&vt->lock);
	}
}

void vt_update(sVTerm *vt) {
	if(!vt->active)
		return;

	locku(&vt->lock);
	vt_doUpdate(vt);
	unlocku(&vt->lock);
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
		if(vt->col != vt->lastCol || vt->row != vt->lastRow) {
			pos.col = vt->col;
			pos.row = vt->row;
			video_setCursor(vt->video,&pos);
			vt->lastCol = vt->col;
			vt->lastRow = vt->row;
		}
	}
}

static int vt_dateThread(A_UNUSED void *arg) {
	size_t i,j,len;
	char dateStr[SSTRLEN("Mon, 14. Jan 2009, 12:13:14") + 1];
	while(1) {
		if(config->enabled) {
			/* get date and format it */
			time_t now = time(NULL);
			struct tm *date = gmtime(&now);
			len = strftime(dateStr,sizeof(dateStr),"%a, %d. %b %Y, %H:%M:%S",date);

			/* update all vterm-title-bars */
			for(i = 0; i < VTERM_COUNT; i++) {
				char *title;
				locku(&vterms[i].lock);
				title = vterms[i].titleBar + (vterms[i].cols - len) * 2;
				for(j = 0; j < len; j++) {
					*title++ = dateStr[j];
					*title++ = LIGHTGRAY | (BLUE << 4);
				}
				if(vterms[i].active) {
					if(seek(vterms[i].video,(vterms[i].cols - len) * 2,SEEK_SET) < 0)
						printe("[VTERM] Unable to seek in video-device");
					if(write(vterms[i].video,vterms[i].titleBar + (vterms[i].cols - len) * 2,len * 2) < 0)
						printe("[VTERM] Unable to write to video-device");
				}
				unlocku(&vterms[i].lock);
			}
		}

		/* wait a second */
		sleep(1000);
	}
	return EXIT_SUCCESS;
}
