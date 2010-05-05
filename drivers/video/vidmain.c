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
#include <esc/driver.h>
#include <esc/io.h>
#include <stdio.h>
#include <esc/ports.h>
#include <messages.h>
#include <esc/heap.h>
#include <esc/mem.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/vm86.h>
#include <string.h>
#include <errors.h>

/* the physical memory of the 80x25 device */
#define VIDEO_MEM				0xB8000

/* cursor io-ports and data bit-masks */
#define CURSOR_PORT_INDEX		0x3D4
#define CURSOR_PORT_DATA		0x3D5
#define CURSOR_DATA_LOCLOW		0x0F
#define CURSOR_DATA_LOCHIGH		0x0E

#define MODE_80x25				0
#define MODE_80x50				1
#define MODE					MODE_80x25

typedef struct {
	u32 cols;
	u32 rows;
	u16 no;
} sVidMode;

/**
 * Sets the video-mode
 */
static void vid_setMode(void);
/**
 * Sets the cursor to given position
 *
 * @param row the row
 * @param col the col
 */
static void vid_setCursor(u8 row,u8 col);

static sVidMode modes[] = {
	{80,25,0x0002},
	{80,50,0x1112}
};
static sVidMode *mode = modes + MODE;

/* our state */
static u8 *videoData;
static sMsg msg;

int main(void) {
	tDrvId id;
	tMsgId mid;

	id = regDriver("video",DRV_WRITE);
	if(id < 0)
		error("Unable to register driver 'video'");

	/* map video-memory for our process */
	videoData = (u8*)mapPhysical(VIDEO_MEM,mode->cols * (mode->rows + 1) * 2);
	if(videoData == NULL)
		error("Unable to aquire video-memory (%p)",VIDEO_MEM);

	/* reserve ports for cursor */
	if(requestIOPorts(CURSOR_PORT_INDEX,2) < 0)
		error("Unable to request ports %d .. %d",CURSOR_PORT_INDEX,CURSOR_PORT_DATA);

	/* clear screen */
	vid_setMode();
	memclear(videoData,mode->cols * mode->rows * 2);

	/* wait for messages */
	while(1) {
		tFD fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[VIDEO] Unable to get work");
		else {
			/* see what we have to do */
			switch(mid) {
				case MSG_DRV_WRITE: {
					u32 offset = msg.args.arg1;
					u32 count = msg.args.arg2;
					msg.args.arg1 = 0;
					if(offset + count <= mode->rows * mode->cols * 2 && offset + count > offset) {
						if(receive(fd,&mid,videoData + offset,count) >= 0)
							msg.args.arg1 = count;
					}
					send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_SETMODE: {
					vid_setMode();
				}
				break;

				case MSG_VID_SETCURSOR: {
					sVTPos *pos = (sVTPos*)msg.data.d;
					pos->col = MIN(pos->col,mode->cols);
					pos->row = MIN(pos->row,mode->rows);
					vid_setCursor(pos->row,pos->col);
					msg.data.arg1 = 0;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
				}
				break;

				case MSG_VID_GETSIZE: {
					sVTSize *size = (sVTSize*)msg.data.d;
					size->width = mode->cols;
					size->height = mode->rows;
					msg.data.arg1 = sizeof(sVTSize);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
				}
				break;
			}
			close(fd);
		}
	}

	/* clean up */
	releaseIOPorts(CURSOR_PORT_INDEX,2);
	unregDriver(id);
	return EXIT_SUCCESS;
}

static void vid_setMode(void) {
	sVM86Regs regs;
	memset(&regs,0,sizeof(regs));
	regs.ax = mode->no;
	if(vm86int(0x10,&regs,NULL,0) < 0)
		printe("Switch to text-mode failed");
}

static void vid_setCursor(u8 row,u8 col) {
   u16 position = (row * mode->cols) + col;

   /* cursor LOW port to vga INDEX register */
   outByte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCLOW);
   outByte(CURSOR_PORT_DATA,(u8)(position & 0xFF));
   /* cursor HIGH port to vga INDEX register */
   outByte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCHIGH);
   outByte(CURSOR_PORT_DATA,(u8)((position >> 8) & 0xFF));
}
