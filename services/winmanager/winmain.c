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
#include "keymap.h"
#include "keymap.ger.h"
#include "keymap.us.h"

#define MOUSE_DATA_BUF_SIZE	128
#define KB_DATA_BUF_SIZE	128

typedef sKeymapEntry *(*fKeymapGet)(u8 keyCode);

/**
 * Destroys the windows of a died thread
 */
static void procThreadHandler(tSig sig,u32 data);
/**
 * Handles a message from keyboard
 */
static void handleKbMessage(tServ servId,sWindow *active,u8 keycode,u8 isBreak);
/**
 * Handles a message from the mouse
 */
static void handleMouseMessage(tServ servId,sMouseData *mdata);

/* our keymaps */
static fKeymapGet keymaps[] = {
	keymap_us_get,
	keymap_ger_get
};

/* mouse state */
static u8 buttons = 0;
static tCoord curX = 0;
static tCoord curY = 0;

/* key states */
static bool shiftDown;
static bool altDown;
static bool ctrlDown;

static sMsg msg;
static tSize screenWidth;
static tSize screenHeight;
static sMouseData mouseData[MOUSE_DATA_BUF_SIZE];
static sKbData kbData[KB_DATA_BUF_SIZE];
static sWindow *mouseWin = NULL;

int main(void) {
	tFD mouse,keyboard;
	tServ servId,client;
	tMsgId mid;

	mouse = open("/drivers/mouse",IO_READ);
	if(mouse < 0)
		error("Unable to open /drivers/mouse");

	keyboard = open("/drivers/keyboard",IO_READ);
	if(keyboard < 0)
		error("Unable to open /drivers/keyboard");

	if(setSigHandler(SIG_THREAD_DIED,procThreadHandler) < 0)
		error("Unable to set sig-handler for %d",SIG_THREAD_DIED);

	servId = regService("winmanager",SERV_DEFAULT);
	if(servId < 0)
		error("Unable to create service winmanager");

	if(!win_init(servId))
		return EXIT_FAILURE;

	screenWidth = win_getScreenWidth();
	screenHeight = win_getScreenHeight();

	while(1) {
		tFD fd = getClient(&servId,1,&client);
		if(fd >= 0) {
			while(receive(fd,&mid,&msg,sizeof(msg)) > 0) {
				switch(mid) {
					case MSG_WIN_CREATE_REQ: {
						u16 x = (u16)(msg.args.arg1 >> 16);
						u16 y = (u16)(msg.args.arg1 & 0xFFFF);
						u16 width = (u16)(msg.args.arg2 >> 16);
						u16 height = (u16)(msg.args.arg2 & 0xFFFF);
						u16 tmpWinId = (u16)msg.args.arg3;
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
						u16 x = (u16)msg.args.arg2;
						u16 y = (u16)msg.args.arg3;
						if(win_exists(wid) && x < screenWidth && y < screenHeight)
							win_moveTo(wid,x,y);
					}
					break;

					case MSG_WIN_UPDATE_REQ: {
						tWinId wid = (tWinId)msg.args.arg1;
						u16 x = (u16)msg.args.arg2;
						u16 y = (u16)msg.args.arg3;
						u16 width = (u16)msg.args.arg4;
						u16 height = (u16)msg.args.arg5;
						sWindow *win = win_get(wid);
						/*debugf("win=%x (%d) @%d,%d s=%d,%d\n",win,data.window,data.x,data.y,
								data.width,data.height);*/
						if(win != NULL && x + width > x && y + height > y &&
							x + width <= win->width && y + height <= win->height) {
							win_update(wid,x,y,width,height);
						}
					}
					break;
				}
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
		else if(!eof(keyboard)) {
			sKbData *kbd = kbData;
			sWindow *active = win_getActive();
			u32 count = read(keyboard,kbData,sizeof(kbData));
			if(active) {
				count /= sizeof(sKbData);
				while(count-- > 0) {
					/*debugf("kc=%d, brk=%d\n",kbd->keycode,kbd->isBreak);*/
					handleKbMessage(servId,active,kbd->keycode,kbd->isBreak);
					kbd++;
				}
			}
		}
		else {
			wait(EV_RECEIVED_MSG | EV_CLIENT);
		}
	}

	unregService(servId);
	close(keyboard);
	close(mouse);
	return EXIT_SUCCESS;
}

static void procThreadHandler(tSig sig,u32 data) {
	UNUSED(sig);
	win_destroyWinsOf(data,curX,curY);
}

static void handleKbMessage(tServ servId,sWindow *active,u8 keycode,u8 isBreak) {
	/* handle shift, alt and ctrl */
	switch(keycode) {
		case VK_LSHIFT:
		case VK_RSHIFT:
			shiftDown = !isBreak;
			break;
		case VK_LALT:
		case VK_RALT:
			altDown = !isBreak;
			break;
		case VK_LCTRL:
		case VK_RCTRL:
			ctrlDown = !isBreak;
			break;
	}

	sKeymapEntry *e = keymaps[active->keymap](keycode);
	if(e != NULL) {
		msg.args.arg1 = keycode;
		msg.args.arg2 = isBreak;
		msg.args.arg3 = active->id;
		if(shiftDown)
			msg.args.arg4 = e->shift;
		else if(altDown)
			msg.args.arg4 = e->alt;
		else
			msg.args.arg4 = e->def;
		msg.args.arg5 = 0;
		if(shiftDown)
			msg.args.arg5 |= SHIFT_MASK;
		if(ctrlDown)
			msg.args.arg5 |= CTRL_MASK;
		if(altDown)
			msg.args.arg5 |= ALT_MASK;

		tFD aWin = getClientThread(servId,active->owner);
		if(aWin >= 0) {
			send(aWin,MSG_WIN_KEYBOARD,&msg,sizeof(msg.args));
			close(aWin);
		}
	}
}

static void handleMouseMessage(tServ servId,sMouseData *mdata) {
	tCoord oldx = curX,oldy = curY;
	bool btnChanged = false;
	curX = MAX(0,MIN(screenWidth - 1,curX + mdata->x));
	curY = MAX(0,MIN(screenHeight - 1,curY - mdata->y));

	/* let vesa draw the cursor */
	if(curX != oldx || curY != oldy)
		win_setCursor(curX,curY);

	/* set active window */
	if(mdata->buttons != buttons) {
		btnChanged = true;
		buttons = mdata->buttons;
		if(buttons) {
			sWindow *w = win_getAt(curX,curY);
			if(w)
				win_setActive(w->id,true,curX,curY);
			else
				win_setActive(WINDOW_COUNT,false,curX,curY);
			mouseWin = w;
		}
	}

	/* send to window */
	sWindow *w = mouseWin ? mouseWin : win_getActive();
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
