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
#include <string.h>

int screen_setCursor(int fd,gpos_t x,gpos_t y,int cursor) {
	sArgsMsg msg;
	msg.arg1 = x;
	msg.arg2 = y;
	msg.arg3 = cursor;
	return send(fd,MSG_SCR_SETCURSOR,&msg,sizeof(msg));
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
	strnzcpy(msg.s1,shm,sizeof(msg.s1));
	int res = send(fd,MSG_SCR_SETMODE,&msg,sizeof(msg));
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg)));
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
	int res = send(fd,MSG_SCR_UPDATE,&msg,sizeof(msg));
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg)));
	if(res < 0)
		return res;
	return msg.arg1;
}

ssize_t screen_getModeCount(int fd) {
	sMsg msg;
	msg.args.arg1 = 0;
	ssize_t res = send(fd,MSG_SCR_GETMODES,&msg,sizeof(msg.args));
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg.args)));
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
