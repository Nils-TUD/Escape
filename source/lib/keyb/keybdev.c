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
#include <esc/driver/vterm.h>
#include <esc/driver/video.h>
#include <esc/keycodes.h>
#include <esc/messages.h>
#include <esc/debug.h>
#include <esc/driver.h>
#include <stdlib.h>

#include <keyb/keybdev.h>

#define MAX_CLIENTS					2

/* file-descriptor for ourself */
static sMsg msg;
static inode_t clients[MAX_CLIENTS];
static int id;

static size_t keyb_find_client(inode_t cid);
static void keyb_broadcast(sKbData *data);
static void keyb_startDebugger(void);

void keyb_driverLoop(void) {
	id = createdev("/dev/keyboard",DEV_TYPE_CHAR,DEV_OPEN | DEV_CLOSE);
	if(id < 0)
		error("Unable to register device 'keyboard'");

	/* wait for commands */
	while(1) {
		inode_t cid;
		msgid_t mid;
		int fd = getwork(id,&cid,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != -EINTR)
				printe("[KB] Unable to get work");
		}
		else {
			switch(mid) {
				case MSG_DEV_OPEN: {
					size_t i = keyb_find_client(0);
					if(i == MAX_CLIENTS)
						msg.args.arg1 = -ENOMEM;
					else {
						clients[i] = cid;
						msg.args.arg1 = 0;
					}
					send(fd,MSG_DEV_OPEN_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_CLOSE: {
					size_t i = keyb_find_client(cid);
					if(i != MAX_CLIENTS)
						clients[i] = 0;
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
	close(id);
}

void keyb_handleKey(sKbData *data) {
	/* F12 starts the kernel-debugging-console */
	if(!data->isBreak && data->keycode == VK_F12)
		keyb_startDebugger();
	else
		keyb_broadcast(data);
}

static size_t keyb_find_client(inode_t cid) {
	size_t i;
	for(i = 0; i < MAX_CLIENTS; ++i) {
		if(clients[i] == cid)
			return i;
	}
	return MAX_CLIENTS;
}

static void keyb_broadcast(sKbData *data) {
	for(size_t i = 0; i < MAX_CLIENTS; ++i) {
		if(clients[i]) {
			int fd = getclient(id,clients[i]);
			if(fd >= 0) {
				send(fd,MSG_KM_EVENT,data,sizeof(*data));
				close(fd);
			}
		}
	}
}

static void keyb_startDebugger(void) {
	/* switch to vga-text-mode */
	int fd = open("/dev/video",IO_MSGS);
	if(fd >= 0) {
		video_setMode(fd,video_getMode(fd));
		close(fd);
	}

	/* disable vterm to prevent that it writes to VESA memory or so (might write garbage to VGA) */
	fd = open("/dev/vterm0",IO_MSGS);
	if(fd >= 0)
		vterm_setEnabled(fd,false);

	/* start debugger */
	debug();

	/* restore video-mode */
	/* TODO this is not perfect since it causes problems when we're in GUI-mode.
	 * But its for debugging, so its ok, I think :) */
	if(fd >= 0) {
		vterm_setEnabled(fd,true);
		close(fd);
	}
}
