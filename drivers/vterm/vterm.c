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
#include <esc/lock.h>
#include <esc/thread.h>
#include <esc/fileio.h>
#include <esc/io.h>
#include <esc/date.h>
#include <esc/keycodes.h>
#include <esc/signals.h>
#include <esc/driver.h>
#include <esc/conf.h>
#include <stdlib.h>
#include <string.h>

#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include "vterm.h"

#define VGA_DRIVER		"/dev/video"
#define VESA_DRIVER		"/dev/vesatext"

/**
 * Handles shortcuts
 */
static bool vterm_handleShortcut(sVTerm *vt,u32 keycode,u8 modifier,char c);
/**
 * Updates the cursor
 */
static void vterm_setCursor(sVTerm *vt);
/**
 * The thread that updates the titlebars every second and puts the date in
 */
static int vterm_dateThread(int argc,char *argv[]);

/* vterms */
static tULock titleBarLock;
static sVTerm vterms[VTERM_COUNT];
static sVTerm *activeVT = NULL;
static sVTermCfg *config;

sVTerm *vterm_get(u32 index) {
	return vterms + index;
}

bool vterm_initAll(tDrvId *ids,sVTermCfg *cfg) {
	tFD vidFd,speakerFd;
	sIoCtlSize vidSize;
	char name[MAX_VT_NAME_LEN + 1];
	const char *driver;
	u32 i;

	config = cfg;

	/* open video-driver */
	if(getConf(CONF_BOOT_VIDEOMODE) == CONF_VIDMODE_VESATEXT)
		driver = VESA_DRIVER;
	else
		driver = VGA_DRIVER;
	vidFd = open(driver,IO_WRITE);
	if(vidFd < 0) {
		printe("Unable to open '%s'",driver);
		return false;
	}

	/* request screensize from video-driver */
	if(recvMsgData(vidFd,IOCTL_VID_GETSIZE,&vidSize,sizeof(sIoCtlSize)) < 0) {
		printe("Getting screensize failed");
		return false;
	}

	/* open speaker */
	speakerFd = open("/dev/speaker",IO_WRITE);
	if(speakerFd < 0) {
		printe("Unable to open '/dev/speaker'");
		return false;
	}

	for(i = 0; i < VTERM_COUNT; i++) {
		vterms[i].index = i;
		vterms[i].sid = ids[i];
		vterms[i].defForeground = LIGHTGRAY;
		vterms[i].defBackground = BLACK;
		snprintf(name,sizeof(name),"vterm%d",i);
		memcpy(vterms[i].name,name,MAX_VT_NAME_LEN + 1);
		if(!vterm_init(vterms + i,&vidSize,vidFd,speakerFd))
			return false;

		vterms[i].handlerShortcut = vterm_handleShortcut;
		vterms[i].setCursor = vterm_setCursor;
	}

	if(startThread(vterm_dateThread,NULL) < 0)
		error("Unable to start date-thread");
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

void vterm_update(sVTerm *vt) {
	u32 byteCount;
	if(!vt->active)
		return;

	/* if we should scroll, mark the whole screen (without title) as dirty */
	if(vt->upScroll != 0) {
		vt->upStart = MIN(vt->upStart,vt->cols * 2);
		vt->upLength = vt->cols * vt->rows * 2 - vt->upStart;
	}

	if(vt->upLength > 0) {
		/* update title-bar? */
		if(vt->upStart < vt->cols * 2) {
			locku(&titleBarLock);
			byteCount = MIN(vt->cols * 2 - vt->upStart,vt->upLength);
			if(seek(vt->video,vt->upStart,SEEK_SET) < 0)
				printe("[VTERM] Unable to seek in video-driver to %d",vt->upStart);
			if(write(vt->video,vt->titleBar,byteCount) < 0)
				printe("[VTERM] Unable to write to video-driver");
			vt->upLength -= byteCount;
			vt->upStart = vt->cols * 2;
			unlocku(&titleBarLock);
		}

		/* refresh the rest */
		byteCount = MIN((vt->rows * vt->cols * 2) - vt->upStart,vt->upLength);
		if(byteCount > 0) {
			char *startPos = vt->buffer + (vt->firstVisLine * vt->cols * 2) + vt->upStart;
			if(seek(vt->video,vt->upStart,SEEK_SET) < 0)
				printe("[VTERM] Unable to seek in video-driver to %d",vt->upStart);
			if(write(vt->video,startPos,byteCount) < 0)
				printe("[VTERM] Unable to write to video-driver");
		}
	}
	vterm_setCursor(vt);

	/* all synchronized now */
	vt->upStart = 0;
	vt->upLength = 0;
	vt->upScroll = 0;
}

static bool vterm_handleShortcut(sVTerm *vt,u32 keycode,u8 modifier,char c) {
	UNUSED(c);
	if(modifier & STATE_CTRL) {
		u32 index;
		switch(keycode) {
			case VK_C:
				/* send interrupt to shell */
				if(vt->shellPid) {
					if(sendSignalTo(vt->shellPid,SIG_INTRPT,0) < 0)
						printe("[VTERM] Unable to send SIG_INTRPT to %d",vt->shellPid);
				}
				break;
			case VK_D:
				if(vt->readLine) {
					vt->inbufEOF = true;
					vterm_rlFlushBuf(vt);
				}
				break;
			case VK_1 ... VK_9:
				index = keycode - VK_1;
				if(index < VTERM_COUNT && vt->index != index)
					vterm_selectVTerm(index);
				return false;
		}
		/* notify the shell (otherwise it won't get the signal directly) */
		if(keycode == VK_C || keycode == VK_D) {
			if(rb_length(vt->inbuf) == 0)
				setDataReadable(vt->sid,true);
			return false;
		}
	}
	return true;
}

static void vterm_setCursor(sVTerm *vt) {
	if(vt->active) {
		sIoCtlPos pos;
		pos.col = vt->col;
		pos.row = vt->row;
		sendMsgData(vt->video,IOCTL_VID_SETCURSOR,&pos,sizeof(pos));
	}
}

static int vterm_dateThread(int argc,char *argv[]) {
	u32 i,j,len;
	char dateStr[SSTRLEN("Mon, 14. Jan 2009, 12:13:14") + 1];
	sDate date;
	UNUSED(argc);
	UNUSED(argv);
	while(1) {
		if(config->refreshDate) {
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
					*title++ = LIGHTGRAY | (BLUE << 4);
				}
				if(vterms[i].active) {
					if(seek(vterms[i].video,(vterms[i].cols - len) * 2,SEEK_SET) < 0)
						printe("[VTERM] Unable to seek in video-driver");
					if(write(vterms[i].video,vterms[i].titleBar + (vterms[i].cols - len) * 2,len * 2) < 0)
						printe("[VTERM] Unable to write to video-driver");
				}
			}
			unlocku(&titleBarLock);
		}

		/* wait a second */
		sleep(1000);
	}
	return EXIT_SUCCESS;
}
