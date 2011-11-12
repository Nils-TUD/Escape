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
#include <esc/driver/keymap.h>
#include <esc/messages.h>
#include <stdio.h>
#include <string.h>

int keymap_set(const char *map) {
	sStrMsg msg;
	int res,fd = open("/dev/kmmanager",IO_MSGS);
	if(fd < 0)
		return fd;
	strnzcpy(msg.s1,map,sizeof(msg.s1));
	res = send(fd,MSG_KM_SET,&msg,sizeof(msg));
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
