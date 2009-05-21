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
#include <esc/messages.h>
#include <esc/keycodes.h>
#include <esc/signals.h>
#include <stdlib.h>
#include "window.h"
#include "keymap.h"
#include "keymap.ger.h"
#include "keymap.us.h"

typedef struct {
	sMsgHeader header;
	sMsgDataWinMouse data;
} __attribute__((packed)) sMsgMouse;

typedef struct {
	sMsgHeader header;
	sMsgDataWinCreateResp data;
} __attribute__((packed)) sMsgWinCreateResp;

typedef struct {
	sMsgHeader header;
	sMsgDataWinKeyboard data;
} __attribute__((packed)) sMsgKeyboard;

typedef sKeymapEntry *(*fKeymapGet)(u8 keyCode);

/**
 * Destroys the windows of a died process
 */
static void procDiedHandler(tSig sig,u32 data);
/**
 * Handles a message from keyboard
 */
static void handleKbMessage(tServ servId,sWindow *active,sMsgKbResponse *msg);

static sMsgWinCreateResp winCreateResp = {
	.header = {
		.id = MSG_WIN_CREATE_RESP,
		.length = sizeof(sMsgDataWinCreateResp)
	}
};
static sMsgMouse mouseMsg = {
	.header = {
		.id = MSG_WIN_MOUSE,
		.length = sizeof(sMsgDataWinMouse)
	}
};
static sMsgKeyboard keyboardMsg = {
	.header = {
		.id = MSG_WIN_KEYBOARD,
		.length = sizeof(sMsgKbResponse)
	}
};

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

static sWindow *mouseWin = NULL;

int main(void) {
	sMsgHeader header;
	sMsgDataMouse mouseData;
	tFD mouse,keyboard;
	tServ servId,client;
	tSize screenWidth,screenHeight;

	mouse = open("services:/mouse",IO_READ);
	if(mouse < 0) {
		printe("Unable to open services:/mouse");
		return EXIT_FAILURE;
	}

	keyboard = open("services:/keyboard",IO_READ);
	if(keyboard < 0) {
		printe("Unable to open services:/keyboard");
		return EXIT_FAILURE;
	}

	if(setSigHandler(SIG_PROC_DIED,procDiedHandler) < 0) {
		printe("Unable to set sig-handler for %d",SIG_PROC_DIED);
		return EXIT_FAILURE;
	}

	servId = regService("winmanager",SERVICE_TYPE_MULTIPIPE);
	if(servId < 0) {
		printe("Unable to create service winmanager");
		return EXIT_FAILURE;
	}

	if(!win_init(servId))
		return EXIT_FAILURE;

	screenWidth = win_getScreenWidth();
	screenHeight = win_getScreenHeight();

	while(1) {
		tFD fd = getClient(&servId,1,&client);
		if(fd >= 0) {
			while(read(fd,&header,sizeof(sMsgHeader)) > 0) {
				switch(header.id) {
					case MSG_WIN_CREATE_REQ: {
						sMsgDataWinCreateReq data;
						if(read(fd,&data,sizeof(data)) == sizeof(data)) {
							winCreateResp.data.tmpId = data.tmpWinId;
							winCreateResp.data.id = win_create(data);
							write(fd,&winCreateResp,sizeof(sMsgWinCreateResp));
							if(data.style == WIN_STYLE_POPUP)
								win_setActive(winCreateResp.data.id,false,curX,curY);
						}
					}
					break;

					case MSG_WIN_DESTROY_REQ: {
						sMsgDataWinDestroyReq data;
						if(read(fd,&data,sizeof(data)) == sizeof(data)) {
							if(win_exists(data.window))
								win_destroy(data.window,curX,curY);
						}
					}
					break;

					case MSG_WIN_MOVE_REQ: {
						sMsgDataWinMoveReq data;
						if(read(fd,&data,sizeof(data)) == sizeof(data)) {
							if(win_exists(data.window) && data.x < screenWidth &&
									data.y < screenHeight) {
								win_moveTo(data.window,data.x,data.y);
							}
						}
					}
					break;
				}
			}
			close(fd);
		}
		/* don't use the blocking read() here */
		else if(!eof(mouse)) {
			if(read(mouse,&header,sizeof(sMsgHeader)) > 0) {
				/* skip invalid data's */
				if(read(mouse,&mouseData,sizeof(sMsgDataMouse)) == sizeof(sMsgDataMouse)) {
					/* TODO extract function */
					tCoord oldx = curX,oldy = curY;
					bool btnChanged = false;
					curX = MAX(0,MIN(screenWidth - 1,curX + mouseData.x));
					curY = MAX(0,MIN(screenHeight - 1,curY - mouseData.y));

					/* let vesa draw the cursor */
					if(curX != oldx || curY != oldy)
						win_setCursor(curX,curY);

					/* set active window */
					if(mouseData.buttons != buttons) {
						btnChanged = true;
						buttons = mouseData.buttons;
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
						tFD aWin = getClientProc(servId,w->owner);
						if(aWin >= 0) {
							mouseMsg.data.x = curX;
							mouseMsg.data.y = curY;
							mouseMsg.data.movedX = mouseData.x;
							mouseMsg.data.movedY = -mouseData.y;
							mouseMsg.data.buttons = mouseData.buttons;
							mouseMsg.data.window = w->id;
							write(aWin,&mouseMsg,sizeof(sMsgMouse));
							close(aWin);
						}
					}

					if(btnChanged && !buttons)
						mouseWin = NULL;
				}
			}
		}
		/* don't use the blocking read() here */
		else if(!eof(keyboard)) {
			sMsgKbResponse keycode;
			if(read(keyboard,&keycode,sizeof(sMsgKbResponse)) == sizeof(sMsgKbResponse)) {
				/* send to active window */
				sWindow *active = win_getActive();
				if(active)
					handleKbMessage(servId,active,&keycode);
			}
		}
		else
			wait(EV_RECEIVED_MSG | EV_CLIENT);
	}

	unregService(servId);
	close(keyboard);
	close(mouse);
	return EXIT_SUCCESS;
}

static void procDiedHandler(tSig sig,u32 data) {
	UNUSED(sig);
	win_destroyWinsOf(data,curX,curY);
}

static void handleKbMessage(tServ servId,sWindow *active,sMsgKbResponse *msg) {
	/* handle shift, alt and ctrl */
	switch(msg->keycode) {
		case VK_LSHIFT:
		case VK_RSHIFT:
			shiftDown = !msg->isBreak;
			break;
		case VK_LALT:
		case VK_RALT:
			altDown = !msg->isBreak;
			break;
		case VK_LCTRL:
		case VK_RCTRL:
			ctrlDown = !msg->isBreak;
			break;
	}

	sKeymapEntry *e = keymaps[active->keymap](msg->keycode);
	if(e != NULL) {
		keyboardMsg.data.keycode = msg->keycode;
		keyboardMsg.data.isBreak = msg->isBreak;
		keyboardMsg.data.window = active->id;
		if(shiftDown)
			keyboardMsg.data.character = e->shift;
		else if(altDown)
			keyboardMsg.data.character = e->alt;
		else
			keyboardMsg.data.character = e->def;
		keyboardMsg.data.modifier = 0;
		if(shiftDown)
			keyboardMsg.data.modifier |= SHIFT_MASK;
		if(ctrlDown)
			keyboardMsg.data.modifier |= CTRL_MASK;
		if(altDown)
			keyboardMsg.data.modifier |= ALT_MASK;

		tFD aWin = getClientProc(servId,active->owner);
		if(aWin >= 0) {
			write(aWin,&keyboardMsg,sizeof(keyboardMsg));
			close(aWin);
		}
	}
}
