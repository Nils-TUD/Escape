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
#include <esc/arch/i586/ports.h>
#include <esc/io.h>
#include <esc/driver.h>
#include <esc/debug.h>
#include <esc/mem.h>
#include <esc/rect.h>
#include <esc/messages.h>
#include <tui/tuidev.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <vbe/vbe.h>

#include "vesatext.h"
#include "vesascreen.h"
#include "font.h"

static size_t modeCount;
static sVTMode *modes;

static int vesacli_setMode(sTUIClient *client,const char *shmname,int mid,bool switchMode) {
	sVbeModeInfo *minfo = NULL;
	if(mid >= 0) {
		minfo = vbe_getModeInfo(modes[mid].id);
		if(minfo == NULL)
			return -EINVAL;
	}

	/* undo previous mapping */
	if(client->shm) {
		munmap(client->shm);
		vesascr_release(client->mode);
	}
	client->shm = NULL;
	client->mode = NULL;

	int res = 0;
	if(minfo) {
		/* request screen */
		sVESAScreen *scr = vesascr_request(minfo);
		if(!scr)
			return -EFAULT;
		vesascr_reset(scr);

		/* set this mode */
		if(switchMode)
			res = vbe_setMode(scr->mode->modeNo);

		/* join shared memory */
		if(res == 0) {
			int fd = shm_open(shmname,IO_READ | IO_WRITE,0);
			if(fd < 0) {
				vesascr_release(scr);
				return fd;
			}
			client->shm = mmap(NULL,minfo->xResolution * minfo->yResolution * (minfo->bitsPerPixel / 8),
				0,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
			close(fd);
			if(client->shm == NULL) {
				vesascr_release(scr);
				return errno;
			}
			client->mode = scr;
			client->cols = scr->cols;
			client->rows = scr->rows;
		}
	}
	return res;
}

static int vesacli_updateScreen(sTUIClient *client,uint start,uint count) {
	sVESAScreen *scr = (sVESAScreen*)client->mode;
	if(!scr)
		return -EINVAL;
	if(start + count < start || start + count > (uint)(scr->cols * scr->rows * 2))
		return -EINVAL;

	vesat_drawChars(scr,(start / 2) % client->cols,
		(start / 2) / client->cols,(uint8_t*)client->shm + start,count / 2);
	return 0;
}

static void vesacli_setCursor(sTUIClient *client,uint row,uint col) {
	if(client->mode)
		vesat_setCursor((sVESAScreen*)client->mode,col,row);
}

int main(void) {
	/* load available modes etc. */
	vbe_init();

	/* get modes */
	vbe_collectModes(0,&modeCount);
	modes = vbe_collectModes(modeCount,&modeCount);
	for(size_t i = 0; i < modeCount; i++) {
		modes[i].width /= FONT_WIDTH + PAD * 2;
		modes[i].height = (modes[i].height - 1) / (FONT_HEIGHT + PAD * 2);
	}

	tui_driverLoop("/dev/vesatext",modes,modeCount,vesacli_setMode,vesacli_setCursor,
		vesacli_updateScreen);
	return EXIT_SUCCESS;
}
