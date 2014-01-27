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
#include <esc/driver/screen.h>
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
#include <vbetext/vbetext.h>

#include "vesatui.h"
#include "vesagui.h"
#include "vesascreen.h"

static size_t modeCount;
static sScreenMode *modes;

static int vesacli_setMode(sTUIClient *client,const char *shmname,int mid,int type,bool switchMode) {
	assert(type == VID_MODE_TYPE_TUI || type == VID_MODE_TYPE_GUI);
	sScreenMode *minfo = NULL;
	if(mid >= 0) {
		minfo = vbe_getModeInfo(modes[mid].id);
		if(minfo == NULL)
			return -EINVAL;
	}

	/* undo previous mapping */
	if(client->shm) {
		munmap(client->shm);
		vesascr_release(client->data);
	}
	client->shm = NULL;
	client->mode = NULL;
	client->type = type;
	client->data = NULL;

	int res = 0;
	if(minfo) {
		/* request screen */
		sVESAScreen *scr = vesascr_request(minfo);
		if(!scr)
			return -EFAULT;

		/* set this mode */
		if(switchMode)
			res = vbe_setMode(scr->mode->id);

		/* join shared memory */
		if(res == 0) {
			res = screen_joinShm(modes + mid,&client->shm,shmname,type);
			if(res < 0) {
				vesascr_release(scr);
				return res;
			}
			/* it worked; reset screen and store new stuff */
			vesascr_reset(scr,type);
			client->mode = modes + mid;
			client->data = scr;
		}
	}
	return res;
}

static int vesacli_updateScreen(sTUIClient *client,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	sVESAScreen *scr = (sVESAScreen*)client->data;
	if(!scr)
		return -EINVAL;

	if(client->type == VID_MODE_TYPE_TUI) {
		if((gpos_t)(x + width) < x || x + width > client->mode->cols ||
			(gpos_t)(y + height) < y || y + height > client->mode->rows)
			return -EINVAL;

		size_t offset = y * client->mode->cols * 2 + x * 2;
		if(width == client->mode->cols)
			vesatui_drawChars(scr,x,y,(uint8_t*)client->shm + offset,width * height);
		else {
			for(gsize_t i = 0; i < height; ++i) {
				vesatui_drawChars(scr,x,y + i,
					(uint8_t*)client->shm + offset + i * client->mode->cols * 2,width);
			}
		}
	}
	else {
		if((gpos_t)(x + width) < x || x + width > scr->mode->width ||
			(gpos_t)(y + height) < y || y + height > scr->mode->height)
			return -EINVAL;

		vesagui_update(scr,client->shm,x,y,width,height);
	}
	return 0;
}

static void vesacli_setCursor(sTUIClient *client,gpos_t x,gpos_t y,int cursor) {
	sVESAScreen *scr = (sVESAScreen*)client->data;
	if(scr) {
		if(client->type == VID_MODE_TYPE_TUI)
			vesatui_setCursor(scr,x,y);
		else
			vesagui_setCursor(scr,client->shm,x,y,cursor);
	}
}

int main(void) {
	/* load available modes etc. */
	vbe_init();
	vesagui_init();

	/* get modes */
	vbe_collectModes(0,&modeCount);
	modes = vbe_collectModes(modeCount,&modeCount);
	for(size_t i = 0; i < modeCount; i++) {
		modes[i].cols = modes[i].width / (FONT_WIDTH + PAD * 2);
		modes[i].rows = (modes[i].height - 1) / (FONT_HEIGHT + PAD * 2);
	}

	tui_driverLoop("/dev/vesa",modes,modeCount,vesacli_setMode,vesacli_setCursor,
		vesacli_updateScreen);
	return EXIT_SUCCESS;
}
