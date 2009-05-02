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
#include <esc/io.h>
#include <esc/service.h>
#include <esc/messages.h>
#include <esc/debug.h>
#include <esc/mem.h>
#include <esc/rect.h>
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
#define BITS_PER_PIXEL					16
#define PIXEL_SIZE						(BITS_PER_PIXEL / 8)
#define VESA_MEM_SIZE					(RESOLUTION_X * RESOLUTION_Y * PIXEL_SIZE)

#define CURSOR_LEN						2
#define CURSOR_COLOR					0xFFFF
#define CURSOR_SIZE						(CURSOR_LEN * 2 + 1)

typedef u16 tSize;
typedef u16 tCoord;
typedef u32 tColor;

static void vbe_update(tCoord x,tCoord y,tSize width,tSize height);
static void vbe_write(u16 index,u16 value);
static void vbe_setMode(tSize xres,tSize yres,u16 bpp);
static void vbe_setCursor(tCoord x,tCoord y);
static void vbe_drawCross(tCoord x,tCoord y);
static void vbe_copyRegion(u8 *src,u8 *dst,tSize width,tSize height,tCoord x1,tCoord y1,
		tCoord x2,tCoord y2,tSize w1,tSize w2);

static void *video;
static void *shmem;
static u8 *cursorCopy;
static tCoord lastX = 0;
static tCoord lastY = 0;

int main(void) {
	tServ id;
	tServ client;

	if(requestIOPort(VBE_DISPI_IOPORT_INDEX) < 0) {
		printe("Unable to request io-port %d",VBE_DISPI_IOPORT_INDEX);
		return EXIT_FAILURE;
	}
	if(requestIOPort(VBE_DISPI_IOPORT_DATA) < 0) {
		printe("Unable to request io-port %d",VBE_DISPI_IOPORT_DATA);
		return EXIT_FAILURE;
	}

	video = mapPhysical(VESA_MEMORY,VESA_MEM_SIZE);
	if(video == NULL) {
		printe("Unable to request physical memory at 0x%x",VESA_MEMORY);
		return EXIT_FAILURE;
	}

	vbe_setMode(RESOLUTION_X,RESOLUTION_Y,BITS_PER_PIXEL);
	shmem = createSharedMem("vesa",VESA_MEM_SIZE);
	if(shmem == NULL) {
		printe("Unable to create shared memory");
		return EXIT_FAILURE;
	}

	cursorCopy = (u8*)malloc(CURSOR_SIZE * CURSOR_SIZE * PIXEL_SIZE);
	if(cursorCopy == NULL) {
		printe("Unable to reserve mem for cursor");
		return EXIT_FAILURE;
	}

	id = regService("vesa",SERVICE_TYPE_SINGLEPIPE);
	if(id < 0) {
		printe("Unable to register service 'vesa'");
		return EXIT_FAILURE;
	}

	while(1) {
		tFD fd = getClient(&id,1,&client);
		if(fd < 0)
			wait(EV_RECEIVED_MSG);
		else {
			sMsgHeader header;
			while(read(fd,&header,sizeof(sMsgHeader)) > 0) {
				switch(header.id) {
					case MSG_VESA_UPDATE: {
						sMsgDataVesaUpdate data;
						read(fd,&data,sizeof(sMsgDataVesaUpdate));
						if(data.x < RESOLUTION_X && data.y < RESOLUTION_Y &&
							data.x + data.width <= RESOLUTION_X &&
							data.y + data.height <= RESOLUTION_Y &&
							/* check for overflow */
							data.x + data.width > data.x &&
							data.y + data.height > data.y) {
							vbe_update(data.x,data.y,data.width,data.height);
						}
					}
					break;

					case MSG_VESA_CURSOR: {
						sMsgDataVesaCursor data;
						read(fd,&data,sizeof(sMsgDataVesaUpdate));
						vbe_setCursor(data.x,data.y);
					}
					break;
				}
			}
			close(fd);
		}
	}

	unregService(id);
	free(cursorCopy);
	destroySharedMem("vesa");
	releaseIOPort(VBE_DISPI_IOPORT_DATA);
	releaseIOPort(VBE_DISPI_IOPORT_INDEX);
	return EXIT_SUCCESS;
}

static void vbe_write(u16 index,u16 value) {
	outWord(VBE_DISPI_IOPORT_INDEX,index);
	outWord(VBE_DISPI_IOPORT_DATA,value);
}

static void vbe_setMode(tSize xres,tSize yres,u16 bpp) {
    vbe_write(VBE_DISPI_INDEX_ENABLE,VBE_DISPI_DISABLED);
    vbe_write(VBE_DISPI_INDEX_XRES,xres);
    vbe_write(VBE_DISPI_INDEX_YRES,yres);
    vbe_write(VBE_DISPI_INDEX_BPP,bpp);
    vbe_write(VBE_DISPI_INDEX_ENABLE,VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
}

static void vbe_update(tCoord x,tCoord y,tSize width,tSize height) {
	static sRectangle upRec,curRec,intersect;
	tCoord y1,y2;
	u32 count;
	u8 *src,*dst;
	y1= y;
	y2 = y + height;
	count = width * PIXEL_SIZE;

	/* look if we have to update the cursor-copy */
	upRec.x = x;
	upRec.y = y;
	upRec.width = width;
	upRec.height = height;
	curRec.x = MAX(0,lastX - CURSOR_LEN);
	curRec.y = MAX(0,lastY - CURSOR_LEN);
	curRec.width = CURSOR_SIZE;
	curRec.height = CURSOR_SIZE;
	if(rectIntersect(&upRec,&curRec,&intersect)) {
		vbe_copyRegion(shmem,cursorCopy,intersect.width,intersect.height,intersect.x,intersect.y,
				intersect.x - curRec.x,intersect.y - curRec.y,RESOLUTION_X,CURSOR_SIZE);
	}

	/* copy from shared-mem to video-mem */
	dst = (u8*)video + (y1 * RESOLUTION_X + x) * PIXEL_SIZE;
	src = (u8*)shmem + (y1 * RESOLUTION_X + x) * PIXEL_SIZE;
	while(y1 < y2) {
		memcpy(dst,src,count);
		src += RESOLUTION_X * PIXEL_SIZE;
		dst += RESOLUTION_X * PIXEL_SIZE;
		y1++;
	}
}

static void vbe_setCursor(tCoord x,tCoord y) {
	tCoord cx,cy;
	/* validate position */
	x = MIN(x,RESOLUTION_X - 1);
	y = MIN(y,RESOLUTION_Y - 1);

	if(lastX != x || lastY != y) {
		cx = MAX(0,lastX - CURSOR_LEN);
		cy = MAX(0,lastY - CURSOR_LEN);
		/* copy old content back */
		vbe_copyRegion(cursorCopy,video,CURSOR_SIZE,CURSOR_SIZE,0,0,cx,cy,CURSOR_SIZE,RESOLUTION_X);
	}
	/* save content */
	cx = MAX(0,x - CURSOR_LEN);
	cy = MAX(0,y - CURSOR_LEN);
	vbe_copyRegion(video,cursorCopy,CURSOR_SIZE,CURSOR_SIZE,cx,cy,0,0,RESOLUTION_X,CURSOR_SIZE);

	vbe_drawCross(x,y);
	lastX = x;
	lastY = y;
}

static void vbe_drawCross(tCoord x,tCoord y) {
	tColor color = CURSOR_COLOR;
	u8 *mid = video + (y * RESOLUTION_X + x) * PIXEL_SIZE;

	/* draw pixel at cursor */
	if(x < RESOLUTION_X && y < RESOLUTION_Y)
		memcpy(mid,&color,PIXEL_SIZE);
	/* draw left */
	if(x >= CURSOR_LEN)
		memcpy(mid - CURSOR_LEN * PIXEL_SIZE,&color,PIXEL_SIZE);
	/* draw top */
	if(y >= CURSOR_LEN)
		memcpy(mid - CURSOR_LEN * RESOLUTION_X * PIXEL_SIZE,&color,PIXEL_SIZE);
	/* draw right */
	if(x < RESOLUTION_X - CURSOR_LEN)
		memcpy(mid + CURSOR_LEN * PIXEL_SIZE,&color,PIXEL_SIZE);
	/* draw bottom */
	if(y < RESOLUTION_Y - CURSOR_LEN)
		memcpy(mid + CURSOR_LEN * RESOLUTION_X * PIXEL_SIZE,&color,PIXEL_SIZE);
}

static void vbe_copyRegion(u8 *src,u8 *dst,tSize width,tSize height,tCoord x1,tCoord y1,
		tCoord x2,tCoord y2,tSize w1,tSize w2) {
	tCoord maxy = y1 + height;
	u32 count = width * PIXEL_SIZE;
	src += (y1 * w1 + x1) * PIXEL_SIZE;
	dst += (y2 * w2 + x2) * PIXEL_SIZE;
	while(y1 < maxy) {
		memcpy(dst,src,count);
		src += w1 * PIXEL_SIZE;
		dst += w2 * PIXEL_SIZE;
		y1++;
	}
}
