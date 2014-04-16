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
#include <esc/arch/i586/vm86.h>
#include <esc/io.h>
#include <esc/mem.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/messages.h>
#include <ipc/screendevice.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vthrow.h>

#define VGA_ADDR				0xB8000
#define VGA_SIZE				80 * 25 * 2

/* cursor io-ports and data bit-masks */
#define CURSOR_PORT_INDEX		0x3D4
#define CURSOR_PORT_DATA		0x3D5
#define CURSOR_DATA_LOCLOW		0x0F
#define CURSOR_DATA_LOCHIGH		0x0E

using namespace ipc;

/* our state */
static uint8_t *vgaData;
static bool usebios = true;

class VGAScreenDevice : public ScreenDevice<> {
public:
	explicit VGAScreenDevice(const std::vector<Screen::Mode> &modes,const char *path,mode_t mode)
		: ScreenDevice(modes,path,mode) {
	}

	virtual void setScreenMode(ScreenClient *c,const char *shm,Screen::Mode *mode,int type,bool sw) {
		if(type != ipc::Screen::MODE_TYPE_TUI)
			throw std::default_error("Invalid mode type");

		if(sw && usebios && mode) {
			int res;
			sVM86Regs regs;
			memclear(&regs,sizeof(regs));
			regs.ax = mode->id;
			/* don't clear the screen */
			regs.ax |= 0x80;
			res = vm86int(0x10,&regs,NULL);
			if(res < 0) {
				errno = res;
				VTHROWE("vm86int",res);
			}
		}

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

	virtual void setScreenCursor(ScreenClient *c,gpos_t x,gpos_t y,int) {
		uint position;
		/* remove cursor, if it is out of bounds */
		if(y >= (gpos_t)c->mode->rows || x >= (gpos_t)c->mode->cols)
			position = 0x7D0;
		else
			position = (y * c->mode->cols) + x;

		/* cursor LOW port to vga INDEX register */
		outbyte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCLOW);
		outbyte(CURSOR_PORT_DATA,(uint8_t)(position & 0xFF));
		/* cursor HIGH port to vga INDEX register */
		outbyte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCHIGH);
		outbyte(CURSOR_PORT_DATA,(uint8_t)((position >> 8) & 0xFF));
	}

	virtual void updateScreen(ScreenClient *c,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		if(!c->mode || !c->fb)
			throw std::default_error("No mode set");
		if((gpos_t)(x + width) < x || x + width > c->mode->cols ||
			(gpos_t)(y + height) < y || y + height > c->mode->rows) {
			VTHROW("Invalid VGA update: " << x << "," << y << ":" << width << "x" << height);
		}

		if(width == c->mode->cols) {
			memcpy(vgaData + y * c->mode->cols * 2,
				c->fb->addr() + y * c->mode->cols * 2,
				height * c->mode->cols * 2);
		}
		else {
			size_t offset = y * c->mode->cols * 2 + x * 2;
			for(gsize_t i = 0; i < height; i++) {
				memcpy(vgaData + offset,c->fb->addr() + offset,width * 2);
				offset += c->mode->cols * 2;
			}
		}
	}
};

int main(int argc,char **argv) {
	if(argc > 1 && strcmp(argv[1],"nobios") == 0) {
		print("Disabled bios-calls; mode-switching will not be possible");
		fflush(stdout);
		usebios = false;
	}

	/* reserve ports for cursor */
	if(reqports(CURSOR_PORT_INDEX,2) < 0)
		error("Unable to request ports %d .. %d",CURSOR_PORT_INDEX,CURSOR_PORT_DATA);

	/* map VGA memory */
	uintptr_t phys = VGA_ADDR;
	vgaData = (uint8_t*)mmapphys(&phys,VGA_SIZE,0);
	if(vgaData == NULL)
		error("Unable to acquire vga-memory (%p)",phys);

	std::vector<Screen::Mode> modes;
	if(usebios) {
		modes.push_back(((Screen::Mode){
			0x0001,40,25,0,0,4,0,0,0,0,0,0,VGA_ADDR,0,0,ipc::Screen::MODE_TEXT,ipc::Screen::MODE_TYPE_TUI
		}));
	}
	modes.push_back(((Screen::Mode){
		0x0003,80,25,0,0,4,0,0,0,0,0,0,VGA_ADDR,0,0,ipc::Screen::MODE_TEXT,ipc::Screen::MODE_TYPE_TUI
	}));

	VGAScreenDevice dev(modes,"/dev/vga",0111);
	dev.loop();

	/* clean up */
	munmap(vgaData);
	relports(CURSOR_PORT_INDEX,2);
	return EXIT_SUCCESS;
}
