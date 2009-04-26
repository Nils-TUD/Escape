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
#include <esc/ports.h>
#include <esc/fileio.h>
#include <esc/debug.h>
#include <esc/mem.h>
#include <stdlib.h>

#define VBE_DISPI_IOPORT_INDEX          0x01CE
#define VBE_DISPI_IOPORT_DATA           0x01CF
#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4
#define VBE_DISPI_INDEX_BANK            0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH      0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     0x7
#define VBE_DISPI_INDEX_X_OFFSET        0x8
#define VBE_DISPI_INDEX_Y_OFFSET        0x9

#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_ENABLED               0x01
#define VBE_DISPI_GETCAPS               0x02
#define VBE_DISPI_8BIT_DAC              0x20
#define VBE_DISPI_LFB_ENABLED           0x40
#define VBE_DISPI_NOCLEARMEM            0x80

#define VESA_MEMORY						0xE0000000
#define RESOLUTION_X					1024
#define RESOLUTION_Y					768
#define BITS_PER_PIXEL					32

static void vbe_write(u16 index,u16 value);
static bool vbe_setMode(u16 xres,u16 yres,u16 bpp);

int main(int argc,char *argv[]) {
	if(requestIOPort(VBE_DISPI_IOPORT_INDEX) < 0) {
		printe("Unable to request io-port %d",VBE_DISPI_IOPORT_INDEX);
		return EXIT_FAILURE;
	}
	if(requestIOPort(VBE_DISPI_IOPORT_DATA) < 0) {
		printe("Unable to request io-port %d",VBE_DISPI_IOPORT_DATA);
		return EXIT_FAILURE;
	}

	u32 *addr = (u32*)mapPhysical(VESA_MEMORY,RESOLUTION_X * RESOLUTION_Y * BITS_PER_PIXEL / 8);
	if(addr == NULL) {
		printe("Unable to request physical memory at 0x%x",VESA_MEMORY);
		return EXIT_FAILURE;
	}

	/*debugf("Setting mode...\n");
	vbe_setMode(1024,768,32);
	debugf("Done\n");*/

	u32 *shaddr = createSharedMem("vesa",RESOLUTION_X * RESOLUTION_Y * BITS_PER_PIXEL / 8);
	debugf("shaddr=0x%x\n",shaddr);

	/*memset(addr,0x00FF00FF,RESOLUTION_X * RESOLUTION_Y * BITS_PER_PIXEL / 8);*/

	while(*shaddr == 0) {
		yield();
	}
	destroySharedMem("vesa");
	return EXIT_SUCCESS;
}

static void vbe_write(u16 index,u16 value) {
	outWord(VBE_DISPI_IOPORT_INDEX,index);
	outWord(VBE_DISPI_IOPORT_DATA,value);
}

static bool vbe_setMode(u16 xres,u16 yres,u16 bpp) {
    /*screen_xres = xres;
    screen_yres = yres;
    screen_bpp = bpp;*/

    vbe_write(VBE_DISPI_INDEX_ENABLE,VBE_DISPI_DISABLED);
    vbe_write(VBE_DISPI_INDEX_XRES,xres);
    vbe_write(VBE_DISPI_INDEX_YRES,yres);
    vbe_write(VBE_DISPI_INDEX_BPP,bpp);
    vbe_write(VBE_DISPI_INDEX_ENABLE,VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
    return true;
}
