/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "window.h"
#include "mouse.h"
#include "keyboard.h"

static sMsg msg;
static tSize screenWidth;
static tSize screenHeight;

int main(void) {
	int drvId;
	msgid_t mid;

	drvId = regDriver("winmanager",DRV_CLOSE);
	if(drvId < 0)
		error("Unable to create driver winmanager");

	if(!win_init(drvId))
		return EXIT_FAILURE;

	screenWidth = win_getScreenWidth();
	screenHeight = win_getScreenHeight();

	if(startThread(mouse_start,&drvId) < 0)
		error("Unable to start thread for mouse-handler");
	if(startThread(keyboard_start,&drvId) < 0)
		error("Unable to start thread for keyboard-handler");

	while(1) {
		int fd = getWork(&drvId,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[WINM] Unable to get work");
		else {
			switch(mid) {
				case MSG_WIN_CREATE: {
					tCoord x = (tCoord)(msg.args.arg1 >> 16);
					tCoord y = (tCoord)(msg.args.arg1 & 0xFFFF);
					tSize width = (tSize)(msg.args.arg2 >> 16);
					tSize height = (tSize)(msg.args.arg2 & 0xFFFF);
					tWinId tmpWinId = (tWinId)msg.args.arg3;
					uint style = msg.args.arg4;
					msg.args.arg1 = tmpWinId;
					msg.args.arg2 = win_create(x,y,width,height,getClientId(fd),style);
					send(fd,MSG_WIN_CREATE_RESP,&msg,sizeof(msg.args));
					if(style == WIN_STYLE_POPUP)
						win_setActive(msg.args.arg2,false,mouse_getX(),mouse_getY());
				}
				break;

				case MSG_WIN_DESTROY: {
					tWinId wid = (tWinId)msg.args.arg1;
					if(win_exists(wid))
						win_destroy(wid,mouse_getX(),mouse_getY());
				}
				break;

				case MSG_WIN_MOVE: {
					tWinId wid = (tWinId)msg.args.arg1;
					tCoord x = (tCoord)msg.args.arg2;
					tCoord y = (tCoord)msg.args.arg3;
					bool finish = (bool)msg.args.arg4;
					sWindow *win = win_get(wid);
					if(win_isEnabled() && win && x < screenWidth && y < screenHeight) {
						if(finish)
							win_moveTo(wid,x,y,win->width,win->height);
						else
							win_previewMove(wid,x,y);
					}
				}
				break;

				case MSG_WIN_RESIZE: {
					tWinId wid = (tWinId)msg.args.arg1;
					tCoord x = (tCoord)msg.args.arg2;
					tCoord y = (tCoord)msg.args.arg3;
					tSize width = (tSize)msg.args.arg4;
					tSize height = (tSize)msg.args.arg5;
					bool finish = (bool)msg.args.arg6;
					if(win_isEnabled() && win_exists(wid)) {
						if(finish)
							win_resize(wid,x,y,width,height);
						else
							win_previewResize(wid,x,y,width,height);
					}
				}
				break;

				case MSG_WIN_UPDATE: {
					tWinId wid = (tWinId)msg.args.arg1;
					tCoord x = (tCoord)msg.args.arg2;
					tCoord y = (tCoord)msg.args.arg3;
					tSize width = (tSize)msg.args.arg4;
					tSize height = (tSize)msg.args.arg5;
					sWindow *win = win_get(wid);
					if(win_isEnabled() && win != NULL && x + width > x && y + height > y &&
						x + width <= win->width && y + height <= win->height) {
						win_update(wid,x,y,width,height);
					}
				}
				break;

				case MSG_WIN_ENABLE:
					win_setEnabled(true);
					win_updateScreen();
					/* notify the keyboard-thread; it has announced the handler */
					sendSignalTo(getpid(),SIG_USR1);
					break;

				case MSG_WIN_DISABLE:
					win_setEnabled(false);
					sendSignalTo(getpid(),SIG_USR1);
					break;

				case MSG_DRV_CLOSE:
					win_destroyWinsOf(getClientId(fd),mouse_getX(),mouse_getY());
					break;

				default:
					msg.args.arg1 = ERR_UNSUPPORTED_OP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}

	close(drvId);
	return EXIT_SUCCESS;
}
