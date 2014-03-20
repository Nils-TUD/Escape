/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <esc/driver.h>
#include <esc/debug.h>
#include <esc/mem.h>
#include <esc/rect.h>
#include <esc/messages.h>
#include <ipc/screendevice.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <memory>

#include <vbe/vbe.h>
#include <vbetext/vbetext.h>

#include "vesatui.h"
#include "vesagui.h"
#include "vesascreen.h"

using namespace ipc;

class VESAClient : public ScreenClient {
public:
	explicit VESAClient(int f) : ScreenClient(f), screen() {
	}

	sVESAScreen *screen;
};

class VESAScreenDevice : public ScreenDevice<VESAClient> {
public:
	explicit VESAScreenDevice(const std::vector<Screen::Mode> &modes,const char *path,mode_t mode)
		: ScreenDevice(modes,path,mode) {
	}

	virtual void setScreenMode(VESAClient *c,const char *shm,Screen::Mode *mode,int type,bool sw) {
		assert(type == VID_MODE_TYPE_TUI || type == VID_MODE_TYPE_GUI);

		/* undo previous mapping */
		if(c->fb)
			delete c->fb;
		if(c->screen)
			vesascr_release(c->screen);
		c->mode = NULL;
		c->fb = NULL;
		c->screen = NULL;

		if(mode) {
			std::unique_ptr<FrameBuffer> fb(new FrameBuffer(*mode,shm,type));

			/* request screen */
			sVESAScreen *scr = vesascr_request(mode);
			if(!scr)
				VTHROW("Unable to request screen");

			/* set this mode */
			if(sw) {
				int res;
				if((res = vbe_setMode(scr->mode->id)) != 0) {
					vesascr_release(scr);
					VTHROWE("Mode setting failed",res);
				}
			}

			/* we're done */
			c->fb = fb.release();
			c->screen = scr;
			c->mode = mode;
			/* it worked; reset screen and store new stuff */
			vesascr_reset(scr,type);
		}
	}

	virtual void setScreenCursor(VESAClient *c,gpos_t x,gpos_t y,int cursor) {
		if(c->screen) {
			if(c->type() == VID_MODE_TYPE_TUI)
				vesatui_setCursor(c->screen,x,y);
			else
				vesagui_setCursor(c->screen,c->fb->addr(),x,y,cursor);
		}
	}

	virtual void updateScreen(VESAClient *c,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		if(!c->mode || !c->fb)
			throw std::default_error("No mode set");

		if(c->type() == VID_MODE_TYPE_TUI) {
			if((gpos_t)(x + width) < x || x + width > c->mode->cols ||
				(gpos_t)(y + height) < y || y + height > c->mode->rows) {
				VTHROW("Invalid TUI update: " << x << "," << y << ":" << width << "x" << height);
			}

			size_t offset = y * c->mode->cols * 2 + x * 2;
			if(width == c->mode->cols)
				vesatui_drawChars(c->screen,x,y,(uint8_t*)c->fb->addr() + offset,width * height);
			else {
				for(gsize_t i = 0; i < height; ++i) {
					vesatui_drawChars(c->screen,x,y + i,
						(uint8_t*)c->fb->addr() + offset + i * c->mode->cols * 2,width);
				}
			}
		}
		else {
			if((gpos_t)(x + width) < x || x + width > c->screen->mode->width ||
				(gpos_t)(y + height) < y || y + height > c->screen->mode->height) {
				VTHROW("Invalid GUI update: " << x << "," << y << ":" << width << "x" << height);
			}

			vesagui_update(c->screen,c->fb->addr(),x,y,width,height);
		}
	}
};

int main(void) {
	/* load available modes etc. */
	vbe_init();
	vesagui_init();

	/* get modes; TODO vector */
	std::vector<Screen::Mode> modes;
	size_t modeCount;
	vbe_collectModes(0,&modeCount);
	Screen::Mode *marray = vbe_collectModes(modeCount,&modeCount);
	for(size_t i = 0; i < modeCount; i++) {
		marray[i].cols = marray[i].width / (FONT_WIDTH + PAD * 2);
		marray[i].rows = (marray[i].height - 1) / (FONT_HEIGHT + PAD * 2);
		modes.push_back(marray[i]);
	}

	VESAScreenDevice dev(modes,"/dev/vesa",0111);
	dev.loop();
	return EXIT_SUCCESS;
}
