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
	ssize_t res = send(fd,MSG_UIM_GETID,NULL,0);
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
	ssize_t res = send(fd,MSG_UIM_ATTACH,&msg,sizeof(msg));
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg)));
	if(res < 0)
		return res;
	return msg.arg1;
}

int uimng_setKeymap(int fd,const char *map) {
	sStrMsg msg;
	strnzcpy(msg.s1,map,sizeof(msg.s1));
	ssize_t res = send(fd,MSG_UIM_SETKEYMAP,&msg,sizeof(msg));
	if(res < 0) {
		close(fd);
		return res;
	}
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg)));
	close(fd);
	if(res < 0)
		return res;
	return msg.arg1;
}
