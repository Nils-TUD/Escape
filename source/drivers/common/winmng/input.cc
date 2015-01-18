/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <esc/proto/winmng.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/esccodes.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/thread.h>
#include <signal.h>
#include <stdlib.h>

#include "input.h"
#include "window.h"

static void handleKbMessage(esc::UIEvents::Event *data);
static void handleMouseMessage(esc::WinMng &winmng,esc::UIEvents::Event *data);

static uchar buttons = 0;
static gpos_t curX = 0;
static gpos_t curY = 0;
static uchar cursor = esc::Screen::CURSOR_DEFAULT;
static Window *mouseWin = NULL;

gpos_t input_getMouseX(void) {
	return curX;
}

gpos_t input_getMouseY(void) {
	return curY;
}

int input_thread(void *arg) {
	sInputThread *in = (sInputThread*)arg;

	/* open ourself to send set-active-requests to us */
	esc::WinMng winmng(in->winmng);

	/* read from uimanager and handle the keys */
	while(1) {
		esc::UIEvents::Event ev;
		*in->uiev >> ev;

		switch(ev.type) {
			case esc::UIEvents::Event::TYPE_KEYBOARD:
				handleKbMessage(&ev);
				break;

			case esc::UIEvents::Event::TYPE_MOUSE:
				handleMouseMessage(winmng,&ev);
				break;

			default:
				break;
		}
	}
	return 0;
}

static void handleKbMessage(esc::UIEvents::Event *data) {
	Window *active = win_getActive();
	if(!active || active->evfd == -1)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_KEYBOARD;
	ev.wid = active->id;
	ev.d.keyb.keycode = data->d.keyb.keycode;
	ev.d.keyb.modifier = data->d.keyb.modifier;
	ev.d.keyb.character = data->d.keyb.character;
	send(active->evfd,MSG_WIN_EVENT,&ev,sizeof(ev));
}

static void handleMouseMessage(esc::WinMng &winmng,esc::UIEvents::Event *data) {
	gpos_t oldx = curX,oldy = curY;
	bool btnChanged = false;
	Window *w,*wheelWin = NULL;
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
				winmng.setActive(w->id);
			}
			mouseWin = w;
		}
	}
	else if(data->d.mouse.z)
		wheelWin = win_getAt(curX,curY);

	/* if no buttons are pressed, change the cursor if we're at a window-border */
	if(!buttons) {
		w = mouseWin ? mouseWin : win_getAt(curX,curY);
		cursor = esc::Screen::CURSOR_DEFAULT;
		if(w && w->style != WIN_STYLE_POPUP && w->style != WIN_STYLE_DESKTOP) {
			gsize_t tbh = w->titleBarHeight;
			bool left = curY >= (gpos_t)(w->y() + tbh) &&
					curX < (gpos_t)(w->x() + esc::Screen::CURSOR_RESIZE_WIDTH);
			bool right = curY >= (gpos_t)(w->y() + tbh) &&
					curX >= (gpos_t)(w->x() + w->width() - esc::Screen::CURSOR_RESIZE_WIDTH);
			bool bottom = curY >= (gpos_t)(w->y() + w->height() - esc::Screen::CURSOR_RESIZE_WIDTH);
			if(left && bottom)
				cursor = esc::Screen::CURSOR_RESIZE_BL;
			else if(left)
				cursor = esc::Screen::CURSOR_RESIZE_L;
			if(right && bottom)
				cursor = esc::Screen::CURSOR_RESIZE_BR;
			else if(right)
				cursor = esc::Screen::CURSOR_RESIZE_R;
			else if(bottom && !left)
				cursor = esc::Screen::CURSOR_RESIZE_VERT;
		}
	}

	/* let vesa draw the cursor */
	if(curX != oldx || curY != oldy)
		win_setCursor(curX,curY,cursor);

	/* send to window */
	w = wheelWin ? wheelWin : (mouseWin ? mouseWin : win_getActive());
	if(w && w->evfd != -1) {
		esc::WinMngEvents::Event ev;
		ev.type = esc::WinMngEvents::Event::TYPE_MOUSE;
		ev.wid = w->id;
		ev.d.mouse.x = curX;
		ev.d.mouse.y = curY;
		ev.d.mouse.movedX = data->d.mouse.x;
		ev.d.mouse.movedY = -data->d.mouse.y;
		ev.d.mouse.movedZ = data->d.mouse.z;
		ev.d.mouse.buttons = data->d.mouse.buttons;
		send(w->evfd,MSG_WIN_EVENT,&ev,sizeof(ev));
	}

	if(btnChanged && !buttons)
		mouseWin = NULL;
}
