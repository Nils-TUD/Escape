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
#include <esc/arch/i586/vm86.h>
#include <esc/driver/screen.h>
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/mem.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/messages.h>
#include <tui/tuidev.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define VGA_ADDR				0xB8000
#define VGA_SIZE				80 * 25 * 2

/* cursor io-ports and data bit-masks */
#define CURSOR_PORT_INDEX		0x3D4
#define CURSOR_PORT_DATA		0x3D5
#define CURSOR_DATA_LOCLOW		0x0F
#define CURSOR_DATA_LOCHIGH		0x0E

static sScreenMode modes[] = {
	{0x0001,40,25,0,0,4,0,0,0,0,0,0,VGA_ADDR,0,0,VID_MODE_TEXT,VID_MODE_TYPE_TUI},
	{0x0003,80,25,0,0,4,0,0,0,0,0,0,VGA_ADDR,0,0,VID_MODE_TEXT,VID_MODE_TYPE_TUI},
};

/* our state */
static uint8_t *vgaData;
static bool usebios = true;

static int vga_setMode(sTUIClient *client,const char *shmname,int mid,int type,bool switchMode) {
	assert(type == VID_MODE_TYPE_TUI);
	if(switchMode && usebios && mid >= 0) {
		int res;
		sVM86Regs regs;
		memclear(&regs,sizeof(regs));
		regs.ax = modes[mid].id;
		/* don't clear the screen */
		regs.ax |= 0x80;
		res = vm86int(0x10,&regs,NULL);
		if(res < 0) {
			printe("Unable to set vga-mode");
			return res;
		}
	}

	/* undo previous mapping */
	if(client->shm)
		munmap(client->shm);
	client->mode = NULL;
	client->shm = NULL;
	client->type = type;

	if(mid >= 0) {
		if(*shmname) {
			/* join shared memory */
			int res = screen_joinShm(modes + mid,&client->shm,shmname,type);
			if(res < 0)
				return res;
		}
		client->mode = modes + mid;
	}
	return 0;
}

static void vga_setCursor(sTUIClient *client,gpos_t x,gpos_t y,A_UNUSED int cursor) {
	uint position;
	/* remove cursor, if it is out of bounds */
	if(y >= (gpos_t)client->mode->rows || x >= (gpos_t)client->mode->cols)
		position = 0x7D0;
	else
		position = (y * client->mode->cols) + x;

	/* cursor LOW port to vga INDEX register */
	outbyte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCLOW);
	outbyte(CURSOR_PORT_DATA,(uint8_t)(position & 0xFF));
	/* cursor HIGH port to vga INDEX register */
	outbyte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCHIGH);
	outbyte(CURSOR_PORT_DATA,(uint8_t)((position >> 8) & 0xFF));
}

static int vga_updateScreen(sTUIClient *client,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	sScreenMode *mode = client->mode;
	if(!mode || !client->shm)
		return -EINVAL;
	if((gpos_t)(x + width) < x || x + width > mode->cols ||
		(gpos_t)(y + height) < y || y + height > mode->rows)
		return -EINVAL;

	if(width == mode->cols)
		memcpy(vgaData + y * mode->cols * 2,client->shm + y * mode->cols * 2,height * mode->cols * 2);
	else {
		size_t offset = y * mode->cols * 2 + x * 2;
		for(gsize_t i = 0; i < height; i++) {
			memcpy(vgaData + offset,client->shm + offset,width * 2);
			offset += mode->cols * 2;
		}
	}
	return 0;
}

int main(int argc,char **argv) {
	if(argc > 1 && strcmp(argv[1],"nobios") == 0) {
		printf("[VGA] Disabled bios-calls; mode-switching will not be possible\n");
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

	sScreenMode *availModes = usebios ? modes : modes + 1;
	size_t availModeCnt = usebios ? ARRAY_SIZE(modes) : 1;
	tui_driverLoop("/dev/vga",availModes,availModeCnt,vga_setMode,vga_setCursor,vga_updateScreen);

	/* clean up */
	munmap(vgaData);
	relports(CURSOR_PORT_INDEX,2);
	return EXIT_SUCCESS;
}
