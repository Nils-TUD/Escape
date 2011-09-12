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
#include <esc/driver/init.h>
#include <esc/messages.h>
#include <esc/proc.h>
#include <stdio.h>

static int init_default(msgid_t mid);

int init_reboot(void) {
	return init_default(MSG_INIT_REBOOT);
}

int init_shutdown(void) {
	return init_default(MSG_INIT_SHUTDOWN);
}

int init_iamalive(void) {
	sArgsMsg msg;
	int res,fd = open("/dev/init",IO_MSGS);
	if(fd < 0)
		return fd;
	msg.arg1 = getpid();
	res = send(fd,MSG_INIT_IAMALIVE,&msg,sizeof(msg));
	close(fd);
	return res;
}

static int init_default(msgid_t mid) {
	int res,fd = open("/dev/init",IO_MSGS);
	if(fd < 0)
		return fd;
	res = send(fd,mid,NULL,0);
	close(fd);
	return res;
}
