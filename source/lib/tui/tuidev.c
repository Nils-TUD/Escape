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
#include <esc/driver.h>
#include <esc/messages.h>
#include <tui/tuidev.h>
#include <stdlib.h>

#define MAX_CLIENTS				16

static sVTMode *modes;
static size_t modeCount;
static sTUIClient clients[MAX_CLIENTS];

static size_t tui_find_client(inode_t cid);

void tui_driverLoop(const char *name,sVTMode *modelist,size_t mcount,fSetMode setMode,
					fSetCursor setCursor,fUpdateScreen updateScreen) {
	modes = modelist;
	modeCount = mcount;

	int id = createdev(name,DEV_TYPE_BLOCK,DEV_OPEN | DEV_CLOSE);
	if(id < 0)
		error("[VGA] Unable to register device '%s'",name);

	/* wait for messages */
	while(1) {
		sMsg msg;
		msgid_t mid;

		int fd = getwork(id,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[VGA] Unable to get work");
		else {
			/* see what we have to do */
			switch(mid) {
				case MSG_DEV_OPEN: {
					size_t i = tui_find_client(0);
					if(i == MAX_CLIENTS)
						msg.args.arg1 = -ENOMEM;
					else {
						clients[i].id = fd;
						clients[i].shm = NULL;
						clients[i].mode = NULL;
						msg.args.arg1 = 0;
					}
					send(fd,MSG_DEV_OPEN_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DEV_CLOSE: {
					size_t i = tui_find_client(fd);
					if(i != MAX_CLIENTS) {
						setMode(clients + i,"",-1,false);
						clients[i].id = 0;
						close(fd);
					}
				}
				break;

				case MSG_VID_UPDATE: {
					gpos_t x = (gpos_t)msg.args.arg1;
					gpos_t y = (gpos_t)msg.args.arg2;
					gsize_t width = (gsize_t)msg.args.arg3;
					gsize_t height = (gsize_t)msg.args.arg4;
					size_t i = tui_find_client(fd);
					msg.args.arg1 = -EINVAL;
					if(i != MAX_CLIENTS)
						msg.args.arg1 = updateScreen(clients + i,x,y,width,height);
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_GETMODES: {
					if(msg.args.arg1 == 0) {
						msg.args.arg1 = modeCount;
						send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					}
					else
						send(fd,MSG_DEF_RESPONSE,modes,modeCount * sizeof(sVTMode));
				}
				break;

				case MSG_VID_GETMODE: {
					size_t i = tui_find_client(fd);
					msg.args.arg1 = i != MAX_CLIENTS ? clients[i].modeid : -EINVAL;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_SETMODE: {
					size_t i;
					int modeno = msg.str.arg1;
					bool switchMode = msg.str.arg2;
					msg.args.arg1 = -EINVAL;
					for(i = 0; i < modeCount ; i++) {
						if(modeno == modes[i].id) {
							size_t j = tui_find_client(fd);
							if(j != MAX_CLIENTS)
								msg.args.arg1 = setMode(clients + j,msg.str.s1,i,switchMode);
							break;
						}
					}
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_SETCURSOR: {
					size_t i = tui_find_client(fd);
					if(i != MAX_CLIENTS && clients[i].mode) {
						sVTPos *pos = (sVTPos*)msg.data.d;
						setCursor(clients + i,pos->row,pos->col);
					}
				}
				break;

				case MSG_VID_GETSIZE: {
					size_t i = tui_find_client(fd);
					sVTSize *size = (sVTSize*)msg.data.d;
					size->width = i != MAX_CLIENTS ? clients[i].cols : 0;
					size->height = i != MAX_CLIENTS ? clients[i].rows : 0;
					msg.data.arg1 = sizeof(sVTSize);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
				}
				break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
		}
	}
}

static size_t tui_find_client(inode_t cid) {
	size_t i;
	for(i = 0; i < MAX_CLIENTS; ++i) {
		if(clients[i].id == cid)
			return i;
	}
	return MAX_CLIENTS;
}
