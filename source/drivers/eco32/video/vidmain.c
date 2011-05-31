/**
 * $Id: vidmain.c 867 2011-05-27 16:57:47Z nasmussen $
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
#include <esc/mem.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/messages.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errors.h>

#define VIDEO_MEM				0x30100000

#define COLS					80
#define ROWS					30
#define MAX_COLS				128

static void copy(uint offset,size_t count);
static void clearScreen(void);

/* our state */
static uint32_t *videoData;
static sMsg msg;
static char buffer[COLS * ROWS * 2];

int main(void) {
	tFD id;
	tMsgId mid;

	id = regDriver("video",DRV_WRITE);
	if(id < 0)
		error("Unable to register driver 'video'");

	/* map video-memory for our process */
	videoData = (uint32_t*)mapPhysical(VIDEO_MEM,MAX_COLS * ROWS * 4);
	if(videoData == NULL)
		error("Unable to aquire video-memory (%p)",VIDEO_MEM);

	clearScreen();

	/* wait for messages */
	while(1) {
		tFD fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[VIDEO] Unable to get work");
		else {
			/* see what we have to do */
			switch(mid) {
				case MSG_DRV_WRITE: {
					uint offset = msg.args.arg1;
					size_t count = msg.args.arg2;
					msg.args.arg1 = 0;
					if(offset + count <= ROWS * COLS * 2 && offset + count > offset) {
						if(RETRY(receive(fd,&mid,buffer + offset,count)) >= 0) {
							copy(offset,count);
							msg.args.arg1 = count;
						}
					}
					send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_SETMODE: {
					/* ignored */
				}
				break;

				case MSG_VID_SETCURSOR: {
					/* ignored */
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
			}
			close(fd);
		}
	}

	/* clean up */
	close(id);
	return EXIT_SUCCESS;
}

static void copy(uint offset,size_t count) {
	uint16_t *buf = (uint16_t*)(buffer + offset);
	size_t x = offset % (COLS * 2);
	size_t y = offset / (COLS * 2);
	size_t begin = y;
	uint32_t *screen = videoData + y * MAX_COLS + x;
	for(; count > 0 && y < ROWS; y++) {
		if(y != begin)
			x = 0;
		for(; count > 0 && x < COLS; x++) {
			uint16_t d = *buf++;
			*screen++ = (d >> 8) | (d & 0xFF) << 8;
			count -= 2;
		}
		screen += MAX_COLS - COLS;
	}
}

static void clearScreen(void) {
	size_t x,y;
	uint32_t *screen = videoData;
	for(y = 0; y < ROWS; y++) {
		for(x = 0; x < COLS; x++)
			*screen++ = 0;
		screen += MAX_COLS - COLS;
	}
}

