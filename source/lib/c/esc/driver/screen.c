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
#include <esc/driver/screen.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/mem.h>
#include <string.h>
#include <stdlib.h>

#define ABS(x)			((x) > 0 ? (x) : -(x))

int screen_setCursor(int fd,gpos_t x,gpos_t y,int cursor) {
	sArgsMsg msg;
	msg.arg1 = x;
	msg.arg2 = y;
	msg.arg3 = cursor;
	return send(fd,MSG_SCR_SETCURSOR,&msg,sizeof(msg));
}

static ssize_t screen_collectModes(int fd,sScreenMode **modes) {
	/* get all modes */
	ssize_t res,count = screen_getModeCount(fd);
	if(count < 0)
		return count;
	*modes = (sScreenMode*)malloc(count * sizeof(sScreenMode));
	if(!*modes)
		return -ENOMEM;
	if((res = screen_getModes(fd,*modes,count)) < 0) {
		free(modes);
		return res;
	}
	return count;
}

static int screen_openShm(sScreenMode *mode,char **addr,const char *name,int type,int flags,uint perms) {
	/* open shm */
	int fd = shm_open(name,flags,perms);
	if(fd < 0)
		return fd;
	size_t size;
	if(type == VID_MODE_TYPE_TUI)
		size = mode->cols * (mode->rows + mode->tuiHeaderSize) * 2;
	else
		size = mode->width * (mode->height + mode->guiHeaderSize) * (mode->bitsPerPixel / 8);
	*addr = mmap(NULL,size,0,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
	close(fd);
	if(*addr == NULL)
		return -ENOMEM;
	if(type == VID_MODE_TYPE_TUI)
		*addr += mode->tuiHeaderSize * mode->cols * 2;
	else
		*addr += mode->guiHeaderSize * mode->width * (mode->bitsPerPixel / 8);
	return 0;
}

int screen_joinShm(sScreenMode *mode,char **addr,const char *name,int type) {
	return screen_openShm(mode,addr,name,type,IO_READ | IO_WRITE,0);
}

int screen_createShm(sScreenMode *mode,char **addr,const char *name,int type,uint perms) {
	return screen_openShm(mode,addr,name,type,IO_READ | IO_WRITE | IO_CREATE,perms);
}

int screen_findTextMode(int fd,uint cols,uint rows,sScreenMode *mode) {
	size_t bestmode;
	uint bestdiff = UINT_MAX;
	sScreenMode *modes;
	ssize_t count = screen_collectModes(fd,&modes);
	if(count < 0)
		return count;

	/* search for the best matching mode */
	bestmode = count;
	for(ssize_t i = 0; i < count; i++) {
		if(modes[i].type & VID_MODE_TYPE_TUI) {
			uint pixdiff = ABS((int)(modes[i].rows * modes[i].cols) - (int)(cols * rows));
			if(pixdiff < bestdiff) {
				bestmode = i;
				bestdiff = pixdiff;
			}
		}
	}
	memcpy(mode,modes + bestmode,sizeof(sScreenMode));
	free(modes);
	return 0;
}

int screen_findGraphicsMode(int fd,gsize_t width,gsize_t height,gcoldepth_t bpp,sScreenMode *mode) {
	sScreenMode *modes;
	ssize_t count = screen_collectModes(fd,&modes);
	if(count < 0)
		return count;

	/* search for the best matching mode */
	uint best = 0;
	size_t pixdiff, bestpixdiff = ABS(320 * 200 - width * height);
	size_t depthdiff, bestdepthdiff = 8 >= bpp ? 8 - bpp : (bpp - 8) * 2;
	for(ssize_t i = 0; i < count; i++) {
		if(modes[i].type & VID_MODE_TYPE_GUI) {
			/* exact match? */
			if(modes[i].width == width && modes[i].height == height && modes[i].bitsPerPixel == bpp) {
				best = i;
				break;
			}

			/* Otherwise, compare to the closest match so far, remember if best */
			pixdiff = ABS(modes[i].width * modes[i].height - width * height);
			if(modes[i].bitsPerPixel >= bpp)
				depthdiff = modes[i].bitsPerPixel - bpp;
			else
				depthdiff = (bpp - modes[i].bitsPerPixel) * 2;
			if(bestpixdiff > pixdiff || (bestpixdiff == pixdiff && bestdepthdiff > depthdiff)) {
				best = i;
				bestpixdiff = pixdiff;
				bestdepthdiff = depthdiff;
			}
		}
	}
	memcpy(mode,modes + best,sizeof(sScreenMode));
	free(modes);
	return 0;
}

int screen_getMode(int fd,sScreenMode *mode) {
	sDataMsg msg;
	int res = send(fd,MSG_SCR_GETMODE,NULL,0);
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg)));
	if(res < 0)
		return res;
	if(msg.arg1 == 0)
		memcpy(mode,msg.d,sizeof(sScreenMode));
	return msg.arg1;
}

int screen_setMode(int fd,int type,int mode,const char *shm,bool switchMode) {
	sStrMsg msg;
	msg.arg1 = mode;
	msg.arg2 = type;
	msg.arg3 = switchMode;
	msgid_t mid = MSG_SCR_SETMODE;
	strnzcpy(msg.s1,shm,sizeof(msg.s1));
	ssize_t res = IGNSIGS(sendrecv(fd,&mid,&msg,sizeof(msg)));
	if(res < 0)
		return res;
	return msg.arg1;
}

int screen_update(int fd,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	sArgsMsg msg;
	msg.arg1 = x;
	msg.arg2 = y;
	msg.arg3 = width;
	msg.arg4 = height;
	return send(fd,MSG_SCR_UPDATE,&msg,sizeof(msg));
}

ssize_t screen_getModeCount(int fd) {
	sMsg msg;
	msgid_t mid = MSG_SCR_GETMODES;
	msg.args.arg1 = 0;
	ssize_t res = IGNSIGS(sendrecv(fd,&mid,&msg,sizeof(msg.args)));
	if(res < 0)
		return res;
	return msg.args.arg1;
}

ssize_t screen_getModes(int fd,sScreenMode *modes,size_t count) {
	sArgsMsg msg;
	ssize_t err;
	msg.arg1 = count;
	err = send(fd,MSG_SCR_GETMODES,&msg,sizeof(msg));
	if(err < 0)
		return err;
	return IGNSIGS(receive(fd,NULL,modes,sizeof(sScreenMode) * count));
}
