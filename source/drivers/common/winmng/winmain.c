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
#include <esc/io.h>
#include <esc/debug.h>
#include <esc/driver.h>
#include <esc/keycodes.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/messages.h>
#include <esc/driver/uimng.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "window.h"
#include "input.h"
#include "listener.h"
#include "infodev.h"

static sMsg msg;
static gsize_t screenWidth;
static gsize_t screenHeight;
static sInputThread inputData;

int main(int argc,char *argv[]) {
	int drvId;
	msgid_t mid;
	char path[MAX_PATH_LEN];

	if(argc != 4) {
		fprintf(stderr,"Usage: %s <width> <height> <name>\n",argv[0]);
		return 1;
	}

	/* connect to ui-manager */
	int uimng = open("/dev/uim-ctrl",IO_MSGS);
	if(uimng < 0)
		error("Unable to open uim-ctrl");
	int uimngId = uimng_getId(uimng);
	if(uimngId < 0)
		error("Unable to get uimng id");

	/* create device */
	snprintf(path,sizeof(path),"/dev/%s",argv[3]);
	drvId = createdev(path,DEV_TYPE_SERVICE,DEV_CLOSE);
	if(drvId < 0)
		error("Unable to create device winmanager");

	/* init window stuff */
	inputData.winFd = drvId;
	inputData.uimngFd = uimng;
	inputData.uimngId = uimngId;
	inputData.shmname = argv[3];
	inputData.mode = win_init(drvId,uimng,atoi(argv[1]),atoi(argv[2]),argv[3]);
	if(inputData.mode < 0)
		return EXIT_FAILURE;

	/* start helper threads */
	listener_init(drvId);
	if(startthread(input_thread,&inputData) < 0)
		error("Unable to start thread for mouse-handler");
	if(startthread(infodev_thread,NULL) < 0)
		error("Unable to start thread for the infodev");

	screenWidth = win_getMode()->width;
	screenHeight = win_getMode()->height;

	while(1) {
		int fd = getwork(drvId,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("Unable to get work");
		else {
			switch(mid) {
				case MSG_WIN_CREATE: {
					gpos_t x = (gpos_t)msg.str.arg1;
					gpos_t y = (gpos_t)msg.str.arg2;
					gsize_t width = (gsize_t)msg.str.arg3;
					gsize_t height = (gsize_t)msg.str.arg4;
					gwinid_t tmpWinId = (gwinid_t)msg.str.arg5;
					uint style = msg.str.arg6;
					gsize_t titleBarHeight = msg.str.arg7;
					msg.args.arg1 = tmpWinId;
					msg.args.arg2 = win_create(x,y,width,height,fd,style,titleBarHeight,msg.str.s1);
					send(fd,MSG_WIN_CREATE_RESP,&msg,sizeof(msg.args));
					if(style != WIN_STYLE_DESKTOP)
						win_setActive(msg.args.arg2,false,input_getMouseX(),input_getMouseY());
				}
				break;

				case MSG_WIN_SET_ACTIVE: {
					gwinid_t wid = (gwinid_t)msg.args.arg1;
					sWindow *win = win_get(wid);
					if(win)
						win_setActive(wid,true,input_getMouseX(),input_getMouseY());
				}
				break;

				case MSG_WIN_DESTROY: {
					gwinid_t wid = (gwinid_t)msg.args.arg1;
					if(win_exists(wid))
						win_destroy(wid,input_getMouseX(),input_getMouseY());
				}
				break;

				case MSG_WIN_MOVE: {
					gwinid_t wid = (gwinid_t)msg.args.arg1;
					gpos_t x = (gpos_t)msg.args.arg2;
					gpos_t y = (gpos_t)msg.args.arg3;
					bool finished = (bool)msg.args.arg4;
					sWindow *win = win_get(wid);
					if(win && x < (gpos_t)screenWidth && y < (gpos_t)screenHeight) {
						if(finished)
							win_moveTo(wid,x,y,win->width,win->height);
						else
							win_previewMove(wid,x,y);
					}
				}
				break;

				case MSG_WIN_RESIZE: {
					gwinid_t wid = (gwinid_t)msg.args.arg1;
					gpos_t x = (gpos_t)msg.args.arg2;
					gpos_t y = (gpos_t)msg.args.arg3;
					gsize_t width = (gsize_t)msg.args.arg4;
					gsize_t height = (gsize_t)msg.args.arg5;
					bool finished = (bool)msg.args.arg6;
					if(win_exists(wid)) {
						if(finished) {
							win_resize(wid,x,y,width,height);
							/* wid is already set */
							send(fd,MSG_WIN_RESIZE_RESP,&msg,sizeof(msg.args));
						}
						else
							win_previewResize(x,y,width,height);
					}
				}
				break;

				case MSG_WIN_UPDATE: {
					gwinid_t wid = (gwinid_t)msg.args.arg1;
					gpos_t x = (gpos_t)msg.args.arg2;
					gpos_t y = (gpos_t)msg.args.arg3;
					gsize_t width = (gsize_t)msg.args.arg4;
					gsize_t height = (gsize_t)msg.args.arg5;
					sWindow *win = win_get(wid);
					if(win != NULL) {
						if((gpos_t)(x + width) > x &&
								(gpos_t)(y + height) > y && x + width <= win->width &&
								y + height <= win->height) {
							win_update(wid,x,y,width,height);
							/* wid is already set */
							send(fd,MSG_WIN_UPDATE_RESP,&msg,sizeof(msg.args));
						}
						else {
							printe("Got illegal update-params: %d,%d %zu,%zu",
									x,y,width,height);
						}
					}
				}
				break;

				case MSG_WIN_ADDLISTENER: {
					msgid_t msgid = (msgid_t)msg.args.arg1;
					if(msgid == MSG_WIN_CREATE_EV || msgid == MSG_WIN_DESTROY_EV ||
							msgid == MSG_WIN_ACTIVE_EV)
						listener_add(fd,msgid);
				}
				break;

				case MSG_WIN_REMLISTENER: {
					msgid_t msgid = (msgid_t)msg.args.arg1;
					if(msgid == MSG_WIN_CREATE_EV || msgid == MSG_WIN_DESTROY_EV ||
							msgid == MSG_WIN_ACTIVE_EV)
						listener_remove(fd,msgid);
				}
				break;

				case MSG_SCR_GETMODE: {
					msg.data.arg1 = 0;
					memcpy(msg.data.d,win_getMode(),sizeof(sScreenMode));
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
				}
				break;

				case MSG_DEV_CLOSE:
					win_destroyWinsOf(fd,input_getMouseX(),input_getMouseY());
					close(fd);
					break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
		}
	}

	close(drvId);
	return EXIT_SUCCESS;
}
