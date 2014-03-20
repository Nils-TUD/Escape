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
#include <assert.h>

#define VIDEO_MEM				0x0005000000000000

#define COLS					80
#define ROWS					30
#define MAX_COLS				128

static sScreenMode modes[] = {
	{0x0003,COLS,ROWS,0,0,4,0,0,0,0,0,0,VIDEO_MEM,0,0,VID_MODE_TEXT,VID_MODE_TYPE_TUI},
};

/* our state */
static uint64_t *vgaData;
static gpos_t lastCol = COLS;
static gpos_t lastRow = ROWS;
static uchar color;

static int vga_setMode(sTUIClient *client,const char *shmname,int mid,int type,A_UNUSED bool switchMode) {
	assert(type == VID_MODE_TYPE_TUI);
	assert(mid == 0);
	/* undo previous mapping */
	if(client->shm)
		munmap(client->shm);
	client->mode = NULL;
	client->shm = NULL;
	client->type = type;

	/* join shared memory */
	if(*shmname) {
		int res = screen_joinShm(modes + mid,&client->shm,shmname,type);
		if(res < 0)
			return res;
	}
	client->mode = modes + mid;
	return 0;
}

static void drawCursor(uint row,uint col,uchar curColor) {
	uint64_t *pos = vgaData + row * MAX_COLS + col;
	*pos = (curColor << 8) | (*pos & 0xFF);
}

static void vga_setCursor(A_UNUSED sTUIClient *client,gpos_t x,gpos_t y,A_UNUSED int cursor) {
	if(lastRow < ROWS && lastCol < COLS)
		drawCursor(lastRow,lastCol,color);
	if(y < ROWS && x < COLS) {
		color = *(vgaData + y * MAX_COLS + x) >> 8;
		drawCursor(y,x,0x78);
	}
	lastCol = x;
	lastRow = y;
}

static int vga_updateScreen(sTUIClient *client,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	sScreenMode *mode = client->mode;
	if(!mode || !client->shm)
		return -EINVAL;
	if((gpos_t)(x + width) < x || x + width > mode->cols ||
		(gpos_t)(y + height) < y || y + height > mode->rows)
		return -EINVAL;

	uint64_t *screen = vgaData + y * MAX_COLS + x;
	for(size_t h = 0; h < height; h++) {
		uint16_t *buf = (uint16_t*)(client->shm + (y + h) * mode->cols * 2 + x * 2);
		for(size_t w = 0; w < width; w++) {
			uint16_t d = *buf++;
			screen[w] = (d >> 8) | (d & 0xFF) << 8;
		}
		screen += MAX_COLS;
	}

	if(lastCol < COLS && lastRow < ROWS) {
		/* update color and draw the cursor again if it has been overwritten */
		if(lastCol >= x && lastCol < (gpos_t)(x + width) &&
			lastRow >= y && lastRow < (gpos_t)(y + height)) {
			color = *(vgaData + lastRow * MAX_COLS + lastCol) >> 8;
			drawCursor(lastRow,lastCol,0x78);
		}
	}
	return 0;
}

int main(void) {
	/* map VGA memory */
	uintptr_t phys = VIDEO_MEM;
	vgaData = (uint64_t*)mmapphys(&phys,MAX_COLS * ROWS * sizeof(uint64_t),0);
	if(vgaData == NULL)
		error("Unable to acquire vga-memory (%p)",VIDEO_MEM);

	tui_driverLoop("/dev/vga",modes,ARRAY_SIZE(modes),vga_setMode,vga_setCursor,vga_updateScreen);

	/* clean up */
	munmap(vgaData);
	return EXIT_SUCCESS;
}
