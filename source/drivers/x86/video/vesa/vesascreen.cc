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

#include <sys/common.h>
#include <sys/mman.h>
#include <esc/vthrow.h>
#include <vbe/vbe.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "vesascreen.h"

std::vector<VESAScreen*> VESAScreen::_screens;

VESAScreen::VESAScreen(esc::Screen::Mode *minfo)
	: refs(1), frmbuf(), whOnBlCache(), mode(minfo),
	  cols(minfo->width / (FONT_WIDTH + PAD * 2)),
	  /* leave at least one pixel free for the cursor */
	  rows((minfo->height - 1) / (FONT_HEIGHT + PAD * 2)), lastCol(), lastRow(),
	  content(new uint8_t[cols * rows * 2]) {

	/* map framebuffer */
	size_t size = minfo->width * minfo->height * (minfo->bitsPerPixel / 8);
	uintptr_t phys = minfo->physaddr;
	frmbuf = static_cast<uint8_t*>(mmapphys(&phys,size,0,MAP_PHYS_MAP));
	if(frmbuf == NULL)
		throw esc::default_error("Unable to map framebuffer",-ENOMEM);

	/* init white-on-black cache */
	initWhOnBl();
}

VESAScreen::~VESAScreen() {
	if(frmbuf != NULL)
		munmap(frmbuf);
	delete[] content;
	delete[] whOnBlCache;
	_screens.erase_first(this);
}

void VESAScreen::initWhOnBl() {
	whOnBlCache = new uint8_t[(FONT_WIDTH + PAD * 2) * (FONT_HEIGHT + PAD * 2) *
			(mode->bitsPerPixel / 8) * FONT_COUNT];

	uint8_t *cc = whOnBlCache;
	uint8_t *white = vbet_getColor(WHITE);
	uint8_t *black = vbet_getColor(BLACK);
	for(size_t i = 0; i < FONT_COUNT; i++) {
		for(int y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
			for(int x = 0; x < FONT_WIDTH + PAD * 2; x++) {
				if(y >= PAD && y < FONT_HEIGHT + PAD && x >= PAD && x < FONT_WIDTH + PAD &&
						PIXEL_SET(i,x - PAD,y - PAD)) {
					cc = vbe_setPixelAt(*mode,cc,white);
				}
				else
					cc = vbe_setPixelAt(*mode,cc,black);
			}
		}
	}
}

VESAScreen *VESAScreen::request(esc::Screen::Mode *minfo) {
	/* is there already a screen for that mode? */
	for(auto it = _screens.begin(); it != _screens.end(); ++it) {
		if((*it)->mode->id == minfo->id) {
			(*it)->refs++;
			return *it;
		}
	}

	/* ok, create a new one */
	VESAScreen *scr = new VESAScreen(minfo);
	_screens.push_back(scr);
	return scr;
}

void VESAScreen::reset(int type) {
	lastCol = cols;
	lastRow = rows;
	memclear(frmbuf,mode->width * mode->height * (mode->bitsPerPixel / 8));
	if(type == esc::Screen::MODE_TYPE_TUI) {
		for(int y = 0; y < rows; y++) {
			for(int x = 0; x < cols; x++) {
				content[y * cols * 2 + x * 2] = ' ';
				content[y * cols * 2 + x * 2 + 1] = 0x07;
			}
		}
	}
}

void VESAScreen::release() {
	if(--refs == 0)
		delete this;
}
