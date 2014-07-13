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
#include <sys/arch/x86/ports.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/debug.h>
#include <sys/proc.h>
#include <sys/messages.h>
#include <vbe/vbe.h>
#include <esc/ipc/screendevice.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vthrow.h>

#include "vga.h"

#define VGA_ADDR				0xB8000
#define VGA_SIZE				80 * 25 * 2

using namespace ipc;

/* our state */
uint8_t *VGA::screen;
std::vector<ipc::Screen::Mode> VGA::modes;

void VGA::ScreenDevice::setScreenMode(ScreenClient *c,const char *shm,Screen::Mode *mode,int type,bool sw) {
	if(type != ipc::Screen::MODE_TYPE_TUI)
		throw std::default_error("Invalid mode type");

	if(sw && mode)
		VBE::setMode(mode->id);

	/* undo previous mapping */
	if(c->fb)
		delete c->fb;
	c->mode = NULL;
	c->fb = NULL;

	if(mode) {
		if(*shm)
			c->fb = new FrameBuffer(*mode,shm,type);
		c->mode = mode;
	}
}

void VGA::ScreenDevice::setScreenCursor(ScreenClient *c,gpos_t x,gpos_t y,int) {
	uint position;
	/* remove cursor, if it is out of bounds */
	if(y >= (gpos_t)c->mode->rows || x >= (gpos_t)c->mode->cols)
		position = 0x7D0;
	else
		position = (y * c->mode->cols) + x;

	/* cursor LOW port to vga INDEX register */
	outbyte(PORT_INDEX,DATA_LOCLOW);
	outbyte(PORT_DATA,(uint8_t)(position & 0xFF));
	/* cursor HIGH port to vga INDEX register */
	outbyte(PORT_INDEX,DATA_LOCHIGH);
	outbyte(PORT_DATA,(uint8_t)((position >> 8) & 0xFF));
}

void VGA::ScreenDevice::updateScreen(ScreenClient *c,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	if(!c->mode || !c->fb)
		throw std::default_error("No mode set");
	if((gpos_t)(x + width) < x || x + width > c->mode->cols ||
		(gpos_t)(y + height) < y || y + height > c->mode->rows) {
		VTHROW("Invalid VGA update: " << x << "," << y << ":" << width << "x" << height);
	}

	if(width == c->mode->cols) {
		memcpy(screen + y * c->mode->cols * 2,
			c->fb->addr() + y * c->mode->cols * 2,
			height * c->mode->cols * 2);
	}
	else {
		size_t offset = y * c->mode->cols * 2 + x * 2;
		for(gsize_t i = 0; i < height; i++) {
			memcpy(screen + offset,c->fb->addr() + offset,width * 2);
			offset += c->mode->cols * 2;
		}
	}
}

void VGA::init() {
	/* reserve ports for cursor */
	if(reqports(PORT_INDEX,2) < 0)
		error("Unable to request ports %d .. %d",PORT_INDEX,PORT_DATA);

	/* map VGA memory */
	uintptr_t phys = VGA_ADDR;
	screen = (uint8_t*)mmapphys(&phys,VGA_SIZE,0,MAP_PHYS_MAP);
	if(screen == NULL)
		error("Unable to acquire vga-memory (%p)",phys);

	modes.push_back(((Screen::Mode){
		0x0001,40,25,0,0,4,0,0,0,0,0,0,VGA_ADDR,0,0,ipc::Screen::MODE_TEXT,ipc::Screen::MODE_TYPE_TUI
	}));
	modes.push_back(((Screen::Mode){
		0x0003,80,25,0,0,4,0,0,0,0,0,0,VGA_ADDR,0,0,ipc::Screen::MODE_TEXT,ipc::Screen::MODE_TYPE_TUI
	}));
}

int VGA::run(void *) {
	ScreenDevice dev(modes,"/dev/vga",0111);
	dev.loop();
	return 0;
}
