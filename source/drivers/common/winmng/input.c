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
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/thread.h>
#include <esc/esccodes.h>
#include <esc/driver/uimng.h>
#include <esc/driver/screen.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "input.h"
#include "window.h"

static void handleKbMessage(sUIMData *data);
static void handleMouseMessage(int winFd,sUIMData *data);

static uchar buttons = 0;
static gpos_t curX = 0;
static gpos_t curY = 0;
static uchar cursor = CURSOR_DEFAULT;
static sWindow *mouseWin = NULL;

gpos_t input_getMouseX(void) {
	return curX;
}

gpos_t input_getMouseY(void) {
	return curY;
}

int input_thread(void *arg) {
	sInputThread *in = (sInputThread*)arg;

	/* open ourself to send set-active-requests to us */
	int winMng = open(in->winmng,IO_MSGS);
	if(winMng < 0)
		error("Unable to open '%s'",in->winmng);

	/* read from uimanager and handle the keys */
	while(1) {
		sUIMData uiEvent;
		ssize_t count = IGNSIGS(receive(in->uiminFd,NULL,&uiEvent,sizeof(uiEvent)));
		if(count > 0) {
			switch(uiEvent.type) {
				case KM_EV_KEYBOARD:
					handleKbMessage(&uiEvent);
					break;

				case KM_EV_MOUSE:
					handleMouseMessage(winMng,&uiEvent);
					break;
			}
		}
	}
	close(winMng);
	return 0;
}

static void handleKbMessage(sUIMData *data) {
	sWindow *active = win_getActive();
	if(!active || active->evfd == -1)
		return;

	sArgsMsg msg;
	msg.arg1 = data->d.keyb.keycode;
	msg.arg2 = (data->d.keyb.modifier & STATE_BREAK) ? 1 : 0;
	msg.arg3 = active->id;
	msg.arg4 = data->d.keyb.character;
	msg.arg5 = data->d.keyb.modifier;
	send(active->evfd,MSG_WIN_KEYBOARD_EV,&msg,sizeof(msg));
}

static void handleMouseMessage(int winFd,sUIMData *data) {
	sArgsMsg msg;
	gpos_t oldx = curX,oldy = curY;
	bool btnChanged = false;
	sWindow *w,*wheelWin = NULL;
	curX = MAX(0,MIN((gpos_t)win_getMode()->width - 1,curX + data->d.mouse.x));
	curY = MAX(0,MIN((gpos_t)win_getMode()->height - 1,curY - data->d.mouse.y));

	/* set active window */
	if(data->d.mouse.buttons != buttons) {
		btnChanged = true;
		buttons = data->d.mouse.buttons;
		if(buttons) {
			w = win_getAt(curX,curY);
			if(w) {
				/* do that via message passing so that only one thread performs changes on the
				 * windows */
				msg.arg1 = w->id;
				send(winFd,MSG_WIN_SET_ACTIVE,&msg,sizeof(msg));
			}
			mouseWin = w;
		}
	}
	else if(data->d.mouse.z)
		wheelWin = win_getAt(curX,curY);

	/* if no buttons are pressed, change the cursor if we're at a window-border */
	if(!buttons) {
		w = mouseWin ? mouseWin : win_getAt(curX,curY);
		cursor = CURSOR_DEFAULT;
		if(w && w->style != WIN_STYLE_POPUP && w->style != WIN_STYLE_DESKTOP) {
			gsize_t tbh = w->titleBarHeight;
			bool left = curY >= (gpos_t)(w->y + tbh) &&
					curX < (gpos_t)(w->x + CURSOR_RESIZE_WIDTH);
			bool right = curY >= (gpos_t)(w->y + tbh) &&
					curX >= (gpos_t)(w->x + w->width - CURSOR_RESIZE_WIDTH);
			bool bottom = curY >= (gpos_t)(w->y + w->height - CURSOR_RESIZE_WIDTH);
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
	w = wheelWin ? wheelWin : (mouseWin ? mouseWin : win_getActive());
	if(w && w->evfd != -1) {
		msg.arg1 = curX;
		msg.arg2 = curY;
		msg.arg3 = data->d.mouse.x;
		msg.arg4 = -data->d.mouse.y;
		msg.arg5 = data->d.mouse.z;
		msg.arg6 = data->d.mouse.buttons;
		msg.arg7 = w->id;
		send(w->evfd,MSG_WIN_MOUSE_EV,&msg,sizeof(msg));
	}

	if(btnChanged && !buttons)
		mouseWin = NULL;
}
