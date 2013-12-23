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
#include <esc/keycodes.h>
#include <esc/messages.h>
#include <esc/debug.h>
#include <esc/driver.h>
#include <stdlib.h>

#include <keyb/keybdev.h>

#define MAX_CLIENTS					2

/* file-descriptor for ourself */
static sMsg msg;
static int clients[MAX_CLIENTS];
static int id;

static size_t keyb_find_client(inode_t cid);

void keyb_driverLoop(void) {
	id = createdev("/dev/keyb",0110,DEV_TYPE_CHAR,DEV_OPEN | DEV_CLOSE);
	if(id < 0)
		error("Unable to register device 'keyb'");

	/* wait for commands */
	while(1) {
		msgid_t mid;
		int fd = getwork(id,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != -EINTR)
				printe("Unable to get work");
		}
		else {
			switch(mid) {
				case MSG_DEV_OPEN: {
					size_t i = keyb_find_client(0);
					if(i == MAX_CLIENTS)
						msg.args.arg1 = -ENOMEM;
					else {
						clients[i] = fd;
						msg.args.arg1 = 0;
					}
					send(fd,MSG_DEV_OPEN_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_CLOSE: {
					size_t i = keyb_find_client(fd);
					if(i != MAX_CLIENTS) {
						clients[i] = 0;
						close(fd);
					}
				}
				break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
		}
	}
	close(id);
}

void keyb_broadcast(sKbData *data) {
	for(size_t i = 0; i < MAX_CLIENTS; ++i) {
		if(clients[i])
			send(clients[i],MSG_KB_EVENT,data,sizeof(*data));
	}
}

static size_t keyb_find_client(int fd) {
	size_t i;
	for(i = 0; i < MAX_CLIENTS; ++i) {
		if(clients[i] == fd)
			return i;
	}
	return MAX_CLIENTS;
}
