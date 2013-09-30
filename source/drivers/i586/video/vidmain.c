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

/* cursor io-ports and data bit-masks */
#define CURSOR_PORT_INDEX		0x3D4
#define CURSOR_PORT_DATA		0x3D5
#define CURSOR_DATA_LOCLOW		0x0F
#define CURSOR_DATA_LOCHIGH		0x0E

static uint16_t vid_getMode(void);
static int vid_setMode(int mid,bool clear);
static void vid_setCursor(uint row,uint col);

typedef struct {
	sVTMode *info;
	uintptr_t addr;
} sVGAMode;

static sVTMode modes[] = {
	{0x0001,40,25,4,VID_MODE_TYPE_TEXT},
	{0x0002,80,25,4,VID_MODE_TYPE_TEXT},
};
static sVGAMode vgamodes[] = {
	{modes + 0,0xB8000},
	{modes + 1,0xB8000},
};
static sVGAMode *mode = vgamodes + 1;

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
		error("[VGA] Unable to register device 'video'");

	/* reserve ports for cursor */
	if(reqports(CURSOR_PORT_INDEX,2) < 0)
		error("[VGA] Unable to request ports %d .. %d",CURSOR_PORT_INDEX,CURSOR_PORT_DATA);

	/* use current mode */
	if(usebios)
		modes[1].id = vid_getMode();
	if(vid_setMode(modes[1].id,false) < 0)
		printe("[VGA] Unable to set VGA mode. Disabling BIOS-calls!");

	/* wait for messages */
	while(1) {
		int fd = getwork(id,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[VGA] Unable to get work");
		else {
			/* see what we have to do */
			switch(mid) {
				case MSG_DEV_WRITE: {
					uint offset = msg.args.arg1;
					size_t count = msg.args.arg2;
					msg.args.arg1 = -EINVAL;
					if(mode && offset + count <= mode->info->height * mode->info->width * 2 &&
							offset + count > offset)
						msg.args.arg1 = IGNSIGS(receive(fd,&mid,videoData + offset,count));
					else
						IGNSIGS(receive(fd,NULL,NULL,0));
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_GETMODES: {
					if(msg.args.arg1 == 0) {
						msg.args.arg1 = usebios ? ARRAY_SIZE(modes) : 1;
						send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					}
					else if(usebios)
						send(fd,MSG_DEF_RESPONSE,modes,sizeof(modes));
					else
						send(fd,MSG_DEF_RESPONSE,mode->info,sizeof(sVTMode));
				}
				break;

				case MSG_VID_GETMODE: {
					msg.args.arg1 = mode ? mode->info->id : -EINVAL;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_SETMODE: {
					size_t i;
					int modeno = msg.args.arg1;
					msg.args.arg1 = -EINVAL;
					for(i = 0; i < ARRAY_SIZE(vgamodes); i++) {
						if(modeno == vgamodes[i].info->id) {
							msg.args.arg1 = vid_setMode(usebios ? modeno : mode->info->id,false);
							break;
						}
					}
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_SETCURSOR: {
					if(mode) {
						sVTPos *pos = (sVTPos*)msg.data.d;
						pos->col = MIN(pos->col,mode->info->width);
						pos->row = MIN(pos->row,mode->info->height);
						vid_setCursor(pos->row,pos->col);
					}
				}
				break;

				case MSG_VID_GETSIZE: {
					sVTSize *size = (sVTSize*)msg.data.d;
					size->width = mode ? mode->info->width : 0;
					size->height = mode ? mode->info->height : 0;
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
	munmap(videoData);
	relports(CURSOR_PORT_INDEX,2);
	close(id);
	return EXIT_SUCCESS;
}

static uint16_t vid_getMode(void) {
	sVM86Regs regs;
	memclear(&regs,sizeof(regs));
	regs.ax = 0x0F00;
	if(vm86int(0x10,&regs,NULL) < 0) {
		printe("[VGA] Getting current vga-mode failed");
		/* fall back to default */
		return 3;
	}
	return regs.ax & 0xFF;
}

static int vid_setMode(int mid,bool clear) {
	size_t i;
	if(usebios) {
		int res;
		sVM86Regs regs;
		memclear(&regs,sizeof(regs));
		regs.ax = mid;
		/* don't clear the screen */
		if(!clear)
			regs.ax |= 0x80;
		res = vm86int(0x10,&regs,NULL);
		if(res < 0) {
			printe("[VGA] Unable to set vga-mode. Disabling BIOS-calls!");
			usebios = false;
		}
	}

	/* find mode */
	mode = NULL;
	for(i = 0; i < ARRAY_SIZE(vgamodes); i++) {
		if(vgamodes[i].info->id == mid) {
			mode = vgamodes + i;
			break;
		}
	}
	if(mode == NULL)
		return -EINVAL;

	/* undo previous mapping */
	if(videoData)
		munmap(videoData);

	/* map video-memory for our process */
	videoData = (uint8_t*)regaddphys(&mode->addr,mode->info->width * (mode->info->height + 1) * 2,0);
	if(videoData == NULL)
		error("[VGA] Unable to acquire video-memory (%p)",mode->addr);
	return 0;
}

static void vid_setCursor(uint row,uint col) {
	uint position = (row * mode->info->width) + col;

   /* cursor LOW port to vga INDEX register */
   outbyte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCLOW);
   outbyte(CURSOR_PORT_DATA,(uint8_t)(position & 0xFF));
   /* cursor HIGH port to vga INDEX register */
   outbyte(CURSOR_PORT_INDEX,CURSOR_DATA_LOCHIGH);
   outbyte(CURSOR_PORT_DATA,(uint8_t)((position >> 8) & 0xFF));
}
