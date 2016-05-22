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

#include <sys/arch/x86/ports.h>
#include <esc/ipc/screendevice.h>
#include <esc/proto/pci.h>
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/driver.h>
#include <sys/messages.h>
#include <sys/mman.h>
#include <vbe/vbe.h>
#include <assert.h>
#include <errno.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "vesa.h"
#include "vesagui.h"
#include "vesascreen.h"
#include "vesatui.h"

using namespace esc;

VESAGUI *VESA::gui;
VESATUI *VESA::tui;
std::vector<esc::Screen::Mode> VESA::modes;

void VESA::ScreenDevice::setScreenMode(Client *c,const char *shm,Screen::Mode *mode,int type,bool sw) {
	assert(type == esc::Screen::MODE_TYPE_TUI || type == esc::Screen::MODE_TYPE_GUI);

	/* undo previous mapping */
	if(c->fb)
		delete c->fb;
	if(c->screen)
		c->screen->release();
	c->mode = NULL;
	c->fb = NULL;
	c->screen = NULL;

	if(mode) {
		std::unique_ptr<FrameBuffer> fb(new FrameBuffer(*mode,shm,type));

		/* request screen */
		VESAScreen *scr = VESAScreen::request(mode);

		/* set this mode */
		if(sw)
			VBE::setMode(scr->mode->id);

		/* we're done */
		c->fb = fb.release();
		c->screen = scr;
		c->mode = mode;
		/* it worked; reset screen and store new stuff */
		scr->reset(type);
	}
}

void VESA::ScreenDevice::setScreenCursor(Client *c,gpos_t x,gpos_t y,int cursor) {
	if(c->screen) {
		if(c->type() == esc::Screen::MODE_TYPE_TUI)
			tui->setCursor(c->screen,x,y);
		else
			gui->setCursor(c->screen,c->fb->addr(),x,y,cursor);
	}
}

void VESA::ScreenDevice::updateScreen(Client *c,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	if(!c->mode || !c->fb)
		throw esc::default_error("No mode set");

	if(c->type() == esc::Screen::MODE_TYPE_TUI) {
		if((gpos_t)(x + width) < x || x + width > c->mode->cols ||
			(gpos_t)(y + height) < y || y + height > c->mode->rows) {
			VTHROW("Invalid TUI update: " << x << "," << y << ":" << width << "x" << height);
		}

		size_t offset = y * c->mode->cols * 2 + x * 2;
		if(width == c->mode->cols)
			tui->drawChars(c->screen,x,y,(uint8_t*)c->fb->addr() + offset,width * height);
		else {
			for(gsize_t i = 0; i < height; ++i) {
				tui->drawChars(c->screen,x,y + i,
					(uint8_t*)c->fb->addr() + offset + i * c->mode->cols * 2,width);
			}
		}
	}
	else {
		if((gpos_t)(x + width) < x || x + width > c->screen->mode->width ||
			(gpos_t)(y + height) < y || y + height > c->screen->mode->height) {
			VTHROW("Invalid GUI update: " << x << "," << y << ":" << width << "x" << height);
		}

		gui->update(c->screen,c->fb->addr(),x,y,width,height);
	}
}

void VESA::init() {
	gui = new VESAGUI();
	tui = new VESATUI();

	uintptr_t fbAddr = 0;
	uint mask = VBE::MODE_SUPPORTED | VBE::MODE_GRAPHICS_MODE | VBE::MODE_LIN_FRAME_BUFFER;
	for(auto m = VBE::begin(); m != VBE::end(); ++m) {
		if((m->second->modeAttributes & mask) == mask && m->second->bitsPerPixel >= 16 &&
				m->second->memoryModel == 6) {
			Screen::Mode mode;
			mode.id = m->first;
			mode.width = m->second->xResolution;
			mode.height = m->second->yResolution;
			mode.cols = m->second->xResolution / (FONT_WIDTH + PAD * 2);
			mode.rows = (m->second->yResolution - 1) / (FONT_HEIGHT + PAD * 2);
			mode.bitsPerPixel = m->second->bitsPerPixel;
			mode.redMaskSize = m->second->redMaskSize;
			mode.redFieldPosition = m->second->redFieldPosition;
			mode.greenMaskSize = m->second->greenMaskSize;
			mode.greenFieldPosition = m->second->greenFieldPosition;
			mode.blueMaskSize = m->second->blueMaskSize;
			mode.blueFieldPosition = m->second->blueFieldPosition;
			fbAddr = mode.physaddr = m->second->physBasePtr;
			mode.tuiHeaderSize = 0;
			mode.guiHeaderSize = 0;
			mode.mode = Screen::MODE_GRAPHICAL;
			mode.type = Screen::MODE_TYPE_TUI | Screen::MODE_TYPE_GUI;
			modes.push_back(mode);
		}
	}

	// set the framebuffer to write-combining
	try {
		PCI pci("/dev/pci");
		PCI::Device dev = pci.getByClass(PCI_CLASS,PCI_SUBCLASS);
		for(size_t i = 0; i < ARRAY_SIZE(PCI::Device::bars); ++i) {
			uintptr_t phys = dev.bars[i].addr;
			size_t size = dev.bars[i].size;
			if(dev.bars[i].type == PCI::Bar::BAR_MEM && phys) {
				if(fbAddr >= phys && fbAddr < phys + size) {
					print("Setting physical memory %p..%p to write-combining",phys,phys + size - 1);
					if(mattr(phys,size,MATTR_WC) < 0)
						printe("Unable to set write-combining");
					break;
				}
			}
		}
	}
	catch(const std::exception &e) {
		print("Unable to find graphics card: %s",e.what());
	}
}

int VESA::run(void *) {
	ScreenDevice dev(modes,"/dev/vesa",0100);
	dev.loop();
	return 0;
}
