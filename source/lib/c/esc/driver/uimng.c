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
#include <esc/driver/uimng.h>
#include <esc/io.h>
#include <string.h>

int uimng_getId(int fd) {
	sArgsMsg msg;
	ssize_t res = send(fd,MSG_KM_GETID,NULL,0);
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg)));
	if(res < 0)
		return res;
	return msg.arg1;
}

int uimng_attach(int fd,int id) {
	sArgsMsg msg;
	msg.arg1 = id;
	ssize_t res = send(fd,MSG_KM_ATTACH,&msg,sizeof(msg));
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg)));
	if(res < 0)
		return res;
	return msg.arg1;
}

int uimng_getMode(int fd) {
	sArgsMsg msg;
	ssize_t res = send(fd,MSG_KM_GETMODE,NULL,0);
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg)));
	if(res < 0)
		return res;
	return msg.arg1;
}

int uimng_setMode(int fd,int mode,const char *shm) {
	sStrMsg msg;
	msg.arg1 = mode;
	strnzcpy(msg.s1,shm,sizeof(msg.s1));
	ssize_t res = send(fd,MSG_KM_SETMODE,&msg,sizeof(msg));
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg)));
	if(res < 0)
		return res;
	return msg.arg1;
}

int uimng_setCursor(int fd,const sVTPos *pos) {
	sDataMsg msg;
	memcpy(&msg.d,pos,sizeof(sVTPos));
	return send(fd,MSG_KM_SETCURSOR,&msg,sizeof(msg));
}

int uimng_update(int fd,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	sArgsMsg msg;
	msg.arg1 = x;
	msg.arg2 = y;
	msg.arg3 = width;
	msg.arg4 = height;
	int res = send(fd,MSG_KM_UPDATE,&msg,sizeof(msg));
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg)));
	if(res < 0)
		return res;
	return msg.arg1;
}

ssize_t uimng_getModeCount(int fd) {
	sArgsMsg msg;
	msg.arg1 = 0;
	int res = send(fd,MSG_KM_GETMODES,&msg,sizeof(msg));
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg)));
	if(res < 0)
		return res;
	return msg.arg1;
}

int uimng_getModes(int fd,sVTMode *modes,size_t count) {
	sArgsMsg msg;
	msg.arg1 = count;
	int res = send(fd,MSG_KM_GETMODES,&msg,sizeof(msg));
	if(res < 0)
		return res;
	return IGNSIGS(receive(fd,NULL,modes,sizeof(sVTMode) * count));
}
