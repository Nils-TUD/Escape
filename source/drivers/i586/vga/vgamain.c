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
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/mem.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/messages.h>
#include <tui/tuidev.h>
#include <string.h>
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

static int vga_setMode(sTUIClient *client,const char *shmname,int mid,bool switchMode);
static void vga_setCursor(sTUIClient *client,uint row,uint col);
static int vga_updateScreen(sTUIClient *client,uint start,uint count);

static sVTMode modes[] = {
	{0x0001,40,25,4,VID_MODE_TEXT,VID_MODE_TYPE_TUI},
	{0x0003,80,25,4,VID_MODE_TEXT,VID_MODE_TYPE_TUI},
};

/* our state */
static uint8_t *vgaData;
static bool usebios = true;

int main(int argc,char **argv) {
	if(argc > 1 && strcmp(argv[1],"nobios") == 0) {
		printf("[VGA] Disabled bios-calls; mode-switching will not be possible\n");
		fflush(stdout);
		usebios = false;
	}

	/* reserve ports for cursor */
	if(reqports(CURSOR_PORT_INDEX,2) < 0)
		error("[VGA] Unable to request ports %d .. %d",CURSOR_PORT_INDEX,CURSOR_PORT_DATA);

	/* map VGA memory */
	uintptr_t phys = VGA_ADDR;
	vgaData = (uint8_t*)regaddphys(&phys,VGA_SIZE,0);
	if(vgaData == NULL)
		error("[VGA] Unable to acquire vga-memory (%p)",phys);

	sVTMode *availModes = usebios ? modes : modes + 1;
	size_t availModeCnt = usebios ? ARRAY_SIZE(modes) : 1;
	tui_driverLoop("/dev/vga",availModes,availModeCnt,vga_setMode,vga_setCursor,vga_updateScreen);

	/* clean up */
	munmap(vgaData);
	relports(CURSOR_PORT_INDEX,2);
	return EXIT_SUCCESS;
}

static int vga_setMode(sTUIClient *client,const char *shmname,int mid,bool switchMode) {
	if(switchMode && usebios && mid >= 0) {
		int res;
		sVM86Regs regs;
		memclear(&regs,sizeof(regs));
		regs.ax = modes[mid].id;
		/* don't clear the screen */
		regs.ax |= 0x80;
		res = vm86int(0x10,&regs,NULL);
		if(res < 0) {
			printe("[VGA] Unable to set vga-mode. Disabling BIOS-calls!");
			usebios = false;
		}
	}

	/* undo previous mapping */
	if(client->shm)
		munmap(client->shm);
	client->mode = NULL;
	client->shm = NULL;

	if(mid >= 0) {
		/* join shared memory */
		int fd = shm_open(shmname,IO_READ | IO_WRITE,0);
		if(fd < 0)
			return fd;
		client->shm = mmap(NULL,modes[mid].width * modes[mid].height * 2,0,PROT_READ | PROT_WRITE,
			MAP_SHARED,fd,0);
		close(fd);
		if(client->shm == NULL)
			return errno;
		client->mode = modes + mid;
		client->cols = modes[mid].width;
		client->rows = modes[mid].height;
		client->modeid = modes[mid].id;
	}
	return 0;
}

static void vga_setCursor(sTUIClient *client,uint row,uint col) {
	uint position;
	/* remove cursor, if it is out of bounds */
	if(row >= client->rows || col >= client->cols)
		position = 0x7D0;
	else
		position = (row * client->cols) + col;

	/* cursor LOW port to vga INDEX register */
	outbyte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCLOW);
	outbyte(CURSOR_PORT_DATA,(uint8_t)(position & 0xFF));
	/* cursor HIGH port to vga INDEX register */
	outbyte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCHIGH);
	outbyte(CURSOR_PORT_DATA,(uint8_t)((position >> 8) & 0xFF));
}

static int vga_updateScreen(sTUIClient *client,uint start,uint count) {
	sVTMode *mode = client->mode;
	if(!mode)
		return -EINVAL;
	if(start + count < start || start + count > mode->width * mode->height * 2)
		return -EINVAL;

	memcpy(vgaData + start,client->shm + start,count);
	return 0;
}
