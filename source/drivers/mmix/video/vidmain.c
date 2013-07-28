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
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/mem.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/messages.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define VIDEO_MEM				0x0005000000000000

#define COLS					80
#define ROWS					30
#define MAX_COLS				128

static void drawCursor(uint row,uint col,uchar color);
static void setCursor(uint row,uint col);
static void copy(uint offset,size_t count);

/* our state */
static uint64_t *videoData;
static sMsg msg;
static char buffer[COLS * ROWS * 2];
static uint lastCol = COLS;
static uint lastRow = ROWS;
static uchar color;

int main(void) {
	int id;
	msgid_t mid;
	uintptr_t phys;

	id = createdev("/dev/video",DEV_TYPE_BLOCK,DEV_WRITE);
	if(id < 0)
		error("Unable to register device 'video'");

	/* map video-memory for our process */
	phys = VIDEO_MEM;
	videoData = (uint64_t*)regaddphys(&phys,MAX_COLS * ROWS * 8,0);
	if(videoData == NULL)
		error("Unable to acquire video-memory (%p)",VIDEO_MEM);

	/* wait for messages */
	while(1) {
		int fd = getwork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[VIDEO] Unable to get work");
		else {
			/* see what we have to do */
			switch(mid) {
				case MSG_DEV_WRITE: {
					uint offset = msg.args.arg1;
					size_t count = msg.args.arg2;
					msg.args.arg1 = 0;
					if(offset + count <= ROWS * COLS * 2 && offset + count > offset) {
						if(IGNSIGS(receive(fd,&mid,buffer + offset,count)) >= 0) {
							copy(offset,count);
							msg.args.arg1 = count;
						}
					}
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_SETCURSOR: {
					sVTPos *pos = (sVTPos*)msg.data.d;
					setCursor(pos->row,pos->col);
				}
				break;

				case MSG_VID_GETSIZE: {
					sVTSize *size = (sVTSize*)msg.data.d;
					size->width = COLS;
					size->height = ROWS;
					msg.data.arg1 = sizeof(sVTSize);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
				}
				break;

				case MSG_VID_GETMODES: {
					if(msg.args.arg1 == 0) {
						msg.args.arg1 = 1;
						send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					}
					else {
						sVTMode mode = {0,COLS,ROWS,4,VID_MODE_TYPE_TEXT};
						send(fd,MSG_DEF_RESPONSE,&mode,sizeof(sVTMode));
					}
				}
				break;

				case MSG_VID_GETMODE: {
					msg.args.arg1 = 0;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_SETMODE: {
					/* pretend that we've set it */
					msg.args.arg1 = 0;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}

	/* clean up */
	munmap(videoData);
	close(id);
	return EXIT_SUCCESS;
}

static void drawCursor(uint row,uint col,uchar color) {
	uint64_t *pos = videoData + row * MAX_COLS + col;
	*pos = (color << 8) | (*pos & 0xFF);
}

static void setCursor(uint row,uint col) {
	if(lastRow < ROWS && lastCol < COLS)
		drawCursor(lastRow,lastCol,color);
	if(row < ROWS && col < COLS) {
		color = *(videoData + row * MAX_COLS + col) >> 8;
		drawCursor(row,col,0x78);
	}
	lastCol = col;
	lastRow = row;
}

static void copy(uint offset,size_t count) {
	uint16_t *buf = (uint16_t*)(buffer + offset);
	size_t x = (offset / 2) % COLS;
	size_t y = (offset / 2) / COLS;
	size_t begin = y;
	size_t rem = count;
	uint64_t *screen = videoData + y * MAX_COLS + x;
	for(; rem > 0 && y < ROWS; y++) {
		if(y != begin)
			x = 0;
		for(; rem > 0 && x < COLS; x++) {
			uint16_t d = *buf++;
			*screen++ = (d >> 8) | (d & 0xFF) << 8;
			rem -= 2;
		}
		screen += MAX_COLS - COLS;
	}

	if(lastCol < COLS && lastRow < ROWS) {
		/* update color and draw the cursor again if it has been overwritten */
		size_t cursorOff = lastRow * COLS * 2 + lastCol * 2;
		if(cursorOff >= offset && cursorOff < offset + count) {
			color = *(videoData + lastRow * MAX_COLS + lastCol) >> 8;
			drawCursor(lastRow,lastCol,0x78);
		}
	}
}
