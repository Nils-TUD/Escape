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
#include <esc/fileio.h>
#include <esc/io.h>
#include <esc/debug.h>
#include <esc/service.h>
#include <messages.h>
#include <esc/keycodes.h>
#include <esc/signals.h>
#include <stdlib.h>
#include "window.h"

#define MOUSE_DATA_BUF_SIZE	128
#define KB_DATA_BUF_SIZE	128

/**
 * Destroys the windows of a died thread
 */
static void deadThreadHandler(tSig sig,u32 data);
/**
 * Handles a message from kmmng
 */
static void handleKbMessage(tServ servId,sWindow *active,u8 keycode,bool isBreak,u8 modifier,char c);
/**
 * Handles a message from the mouse
 */
static void handleMouseMessage(tServ servId,sMouseData *mdata);

/* mouse state */
static u8 buttons = 0;
static tCoord curX = 0;
static tCoord curY = 0;
static u8 cursor = CURSOR_DEFAULT;

static sMsg msg;
static tSize screenWidth;
static tSize screenHeight;
static sMouseData mouseData[MOUSE_DATA_BUF_SIZE];
static sKmData kbData[KB_DATA_BUF_SIZE];
static sWindow *mouseWin = NULL;

int main(void) {
	tFD mouse,kmmng;
	tServ servId;
	tMsgId mid;

	mouse = open("/dev/mouse",IO_READ);
	if(mouse < 0)
		error("Unable to open /dev/mouse");

	kmmng = open("/dev/kmmanager",IO_READ);
	if(kmmng < 0)
		error("Unable to open /dev/kmmanager");

	if(setSigHandler(SIG_THREAD_DIED,deadThreadHandler) < 0)
		error("Unable to set sig-handler for %d",SIG_THREAD_DIED);

	servId = regService("winmanager",0);
	if(servId < 0)
		error("Unable to create service winmanager");

	if(!win_init(servId))
		return EXIT_FAILURE;

	screenWidth = win_getScreenWidth();
	screenHeight = win_getScreenHeight();

	while(1) {
		tFD fd = getWork(&servId,1,NULL,&mid,&msg,sizeof(msg),GW_NOBLOCK);
		if(fd >= 0) {
			switch(mid) {
				case MSG_WIN_CREATE_REQ: {
					tCoord x = (tCoord)(msg.args.arg1 >> 16);
					tCoord y = (tCoord)(msg.args.arg1 & 0xFFFF);
					tSize width = (tSize)(msg.args.arg2 >> 16);
					tSize height = (tSize)(msg.args.arg2 & 0xFFFF);
					tWinId tmpWinId = (tWinId)msg.args.arg3;
					tPid owner = (tPid)msg.args.arg4;
					u8 style = (u8)msg.args.arg5;
					msg.args.arg1 = tmpWinId;
					msg.args.arg2 = win_create(x,y,width,height,owner,style);
					send(fd,MSG_WIN_CREATE_RESP,&msg,sizeof(msg.args));
					if(style == WIN_STYLE_POPUP)
						win_setActive(msg.args.arg2,false,curX,curY);
				}
				break;

				case MSG_WIN_DESTROY_REQ: {
					tWinId wid = (tWinId)msg.args.arg1;
					if(win_exists(wid))
						win_destroy(wid,curX,curY);
				}
				break;

				case MSG_WIN_MOVE_REQ: {
					tWinId wid = (tWinId)msg.args.arg1;
					tCoord x = (tCoord)msg.args.arg2;
					tCoord y = (tCoord)msg.args.arg3;
					if(win_exists(wid) && x < screenWidth && y < screenHeight)
						win_moveTo(wid,x,y);
				}
				break;

				case MSG_WIN_RESIZE_REQ: {
					tWinId wid = (tWinId)msg.args.arg1;
					tSize width = (tSize)msg.args.arg2;
					tSize height = (tSize)msg.args.arg3;
					if(win_exists(wid))
						win_resize(wid,width,height);
				}
				break;

				case MSG_WIN_UPDATE_REQ: {
					tWinId wid = (tWinId)msg.args.arg1;
					tCoord x = (tCoord)msg.args.arg2;
					tCoord y = (tCoord)msg.args.arg3;
					tSize width = (tSize)msg.args.arg4;
					tSize height = (tSize)msg.args.arg5;
					sWindow *win = win_get(wid);
					if(win != NULL && x + width > x && y + height > y &&
						x + width <= win->width && y + height <= win->height) {
						win_update(wid,x,y,width,height);
					}
				}
				break;
			}
			close(fd);
		}
		/* don't use the blocking read() here */
		else if(!eof(mouse)) {
			sMouseData *msd = mouseData;
			u32 count = read(mouse,mouseData,sizeof(mouseData));
			count /= sizeof(sMouseData);
			while(count-- > 0) {
				handleMouseMessage(servId,msd);
				msd++;
			}
		}
		/* don't use the blocking read() here */
		else if(!eof(kmmng)) {
			sKmData *kbd = kbData;
			sWindow *active = win_getActive();
			u32 count = read(kmmng,kbData,sizeof(kbData));
			if(active) {
				count /= sizeof(sKmData);
				while(count-- > 0) {
					/*printf("kc=%d, brk=%d\n",kbd->keycode,kbd->isBreak);*/
					handleKbMessage(servId,active,kbd->keycode,kbd->isBreak,kbd->modifier,
							kbd->character);
					kbd++;
				}
			}
		}
		else {
			wait(EV_RECEIVED_MSG | EV_CLIENT);
		}
	}

	unregService(servId);
	close(kmmng);
	close(mouse);
	return EXIT_SUCCESS;
}

static void deadThreadHandler(tSig sig,u32 data) {
	UNUSED(sig);
	/* TODO this is dangerous! we can't use the heap in signal-handlers */
	win_destroyWinsOf(data,curX,curY);
}

static void handleKbMessage(tServ servId,sWindow *active,u8 keycode,bool isBreak,u8 modifier,char c) {
	tFD aWin;
	msg.args.arg1 = keycode;
	msg.args.arg2 = isBreak;
	msg.args.arg3 = active->id;
	msg.args.arg4 = c;
	msg.args.arg5 = modifier;
	aWin = getClientThread(servId,active->owner);
	if(aWin >= 0) {
		send(aWin,MSG_WIN_KEYBOARD,&msg,sizeof(msg.args));
		close(aWin);
	}
}

static void handleMouseMessage(tServ servId,sMouseData *mdata) {
	tCoord oldx = curX,oldy = curY;
	bool btnChanged = false;
	sWindow *w;
	curX = MAX(0,MIN(screenWidth - 1,curX + mdata->x));
	curY = MAX(0,MIN(screenHeight - 1,curY - mdata->y));

	/* set active window */
	if(mdata->buttons != buttons) {
		btnChanged = true;
		buttons = mdata->buttons;
		if(buttons) {
			w = win_getAt(curX,curY);
			if(w)
				win_setActive(w->id,true,curX,curY);
			else
				win_setActive(WINDOW_COUNT,false,curX,curY);
			mouseWin = w;
		}
	}

	/* if no buttons are pressed, change the cursor if we're at a window-border */
	if(!buttons) {
		w = mouseWin ? mouseWin : win_getAt(curX,curY);
		cursor = CURSOR_DEFAULT;
		if(w && w->style != WIN_STYLE_POPUP) {
			bool left = curX < w->x + CURSOR_RESIZE_WIDTH;
			bool right = curX >= w->x + w->width - CURSOR_RESIZE_WIDTH;
			bool bottom = curY >= w->y + w->height - CURSOR_RESIZE_WIDTH;
			if(left && bottom)
				cursor = CURSOR_RESIZE_BL;
			else if(left)
				cursor = CURSOR_RESIZE_L;
			if(right && bottom)
				cursor = CURSOR_RESIZE_BR;
			else if(right)
				cursor = CURSOR_RESIZE_R;
			else if(bottom && !left)
				cursor = CURSOR_RESIZE_VERT;
		}
	}

	/* let vesa draw the cursor */
	if(curX != oldx || curY != oldy)
		win_setCursor(curX,curY,cursor);

	/* send to window */
	w = mouseWin ? mouseWin : win_getActive();
	if(w) {
		tFD aWin = getClientThread(servId,w->owner);
		if(aWin >= 0) {
			msg.args.arg1 = curX;
			msg.args.arg2 = curY;
			msg.args.arg3 = mdata->x;
			msg.args.arg4 = -mdata->y;
			msg.args.arg5 = mdata->buttons;
			msg.args.arg6 = w->id;
			send(aWin,MSG_WIN_MOUSE,&msg,sizeof(msg.args));
			close(aWin);
		}
	}

	if(btnChanged && !buttons)
		mouseWin = NULL;
}
