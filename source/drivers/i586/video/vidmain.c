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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* the physical memory of the 80x25 device */
#define VIDEO_MEM				0xB8000

/* cursor io-ports and data bit-masks */
#define CURSOR_PORT_INDEX		0x3D4
#define CURSOR_PORT_DATA		0x3D5
#define CURSOR_DATA_LOCLOW		0x0F
#define CURSOR_DATA_LOCHIGH		0x0E

static uint16_t vid_getMode(void);
static int vid_setMode(int mid,bool clear);
static void vid_setCursor(uint row,uint col);

static sVTMode modes[] = {
	{0x0002,80,25,2,VID_MODE_TYPE_TEXT},
	{0x1112,80,50,2,VID_MODE_TYPE_TEXT}
};
static sVTMode *mode;

/* our state */
static uint8_t *videoData;
static sMsg msg;
static bool usebios = true;

int main(int argc,char **argv) {
	int id;
	msgid_t mid;

	if(argc > 1 && strcmp(argv[1],"nobios") == 0) {
		printf("[VGA] Disabled bios-calls; mode-switching will not be possible\n");
		fflush(stdout);
		usebios = false;
	}

	id = createdev("/dev/video",DEV_TYPE_BLOCK,DEV_WRITE);
	if(id < 0)
		error("Unable to register device 'video'");

	/* reserve ports for cursor */
	if(reqports(CURSOR_PORT_INDEX,2) < 0)
		error("Unable to request ports %d .. %d",CURSOR_PORT_INDEX,CURSOR_PORT_DATA);

	/* use current mode */
	modes[0].id = vid_getMode();
	if(vid_setMode(modes[1].id,false) < 0)
		printe("Unable to set VGA mode");

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
					if(offset + count <= mode->height * mode->width * 2 && offset + count > offset) {
						if(IGNSIGS(receive(fd,&mid,videoData + offset,count)) >= 0)
							msg.args.arg1 = count;
					}
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_GETMODES: {
					if(msg.args.arg1 == 0) {
						msg.args.arg1 = ARRAY_SIZE(modes);
						send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					}
					else
						send(fd,MSG_DEF_RESPONSE,modes,sizeof(modes));
				}
				break;

				case MSG_VID_GETMODE: {
					msg.args.arg1 = mode->id;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_SETMODE: {
					if(!usebios)
						msg.args.arg1 = -ENOTSUP;
					else {
						size_t i;
						int mode = msg.args.arg1;
						msg.args.arg1 = -EINVAL;
						for(i = 0; i < ARRAY_SIZE(modes); i++) {
							if(mode == modes[i].id) {
								vid_setMode(modes[i].id,false);
								msg.args.arg1 = 0;
								break;
							}
						}
					}
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_SETCURSOR: {
					sVTPos *pos = (sVTPos*)msg.data.d;
					pos->col = MIN(pos->col,mode->width);
					pos->row = MIN(pos->row,mode->height);
					vid_setCursor(pos->row,pos->col);
				}
				break;

				case MSG_VID_GETSIZE: {
					sVTSize *size = (sVTSize*)msg.data.d;
					size->width = mode->width;
					size->height = mode->height;
					msg.data.arg1 = sizeof(sVTSize);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
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
	relports(CURSOR_PORT_INDEX,2);
	close(id);
	return EXIT_SUCCESS;
}

static uint16_t vid_getMode(void) {
	sVM86Regs regs;
	memclear(&regs,sizeof(regs));
	regs.ax = 0x0F00;
	if(vm86int(0x10,&regs,NULL) < 0)
		printe("Getting current vga-mode failed");
	return regs.ax & 0xFF;
}

static int vid_setMode(int mid,bool clear) {
	int res;
	size_t i;
	sVM86Regs regs;
	memclear(&regs,sizeof(regs));
	regs.ax = mid;
	/* don't clear the screen */
	if(!clear)
		regs.ax |= 0x80;
	res = vm86int(0x10,&regs,NULL);
	if(res < 0)
		return res;

	/* find mode */
	mode = NULL;
	for(i = 0; i < ARRAY_SIZE(modes); i++) {
		if(modes[i].id == mid) {
			mode = modes + i;
			break;
		}
	}
	if(mode == NULL)
		return -EINVAL;

	/* map video-memory for our process */
	videoData = (uint8_t*)mapphys(VIDEO_MEM,mode->width * (mode->height + 1) * 2);
	if(videoData == NULL)
		error("Unable to aquire video-memory (%p)",VIDEO_MEM);
	return 0;
}

static void vid_setCursor(uint row,uint col) {
	uint position = (row * mode->width) + col;

   /* cursor LOW port to vga INDEX register */
   outbyte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCLOW);
   outbyte(CURSOR_PORT_DATA,(uint8_t)(position & 0xFF));
   /* cursor HIGH port to vga INDEX register */
   outbyte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCHIGH);
   outbyte(CURSOR_PORT_DATA,(uint8_t)((position >> 8) & 0xFF));
}
