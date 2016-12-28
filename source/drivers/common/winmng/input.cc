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
#include <esc/util.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/esccodes.h>
#include <sys/keycodes.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/thread.h>
#include <signal.h>
#include <stdlib.h>

#include "input.h"
#include "listener.h"
#include "stack.h"
#include "winlist.h"

Input *Input::_inst;

int Input::thread(void *) {
	Input &in = Input::get();
	/* open ourself to send set-active-requests to us */
	esc::WinMng winmng(in._winmng);

	/* read from uimanager and handle the keys */
	while(1) {
		esc::UIEvents::Event ev;
		*in._uiev >> ev;

		switch(ev.type) {
			case esc::UIEvents::Event::TYPE_KEYBOARD:
				in.handleKbMessage(&ev);
				break;

			case esc::UIEvents::Event::TYPE_MOUSE:
				in.handleMouseMessage(winmng,&ev);
				break;

			default:
				break;
		}
	}
	return 0;
}

void Input::handleKbMessage(esc::UIEvents::Event *data) {
	if(data->d.keyb.modifier & STATE_CTRL) {
		if(data->d.keyb.keycode == VK_TAB) {
			if(~data->d.keyb.modifier & STATE_BREAK) {
				if(data->d.keyb.modifier & STATE_SHIFT)
					Stack::prev();
				else
					Stack::next();
			}
			return;
		}
		else if(!(data->d.keyb.modifier & (STATE_SHIFT | STATE_BREAK)))
			Stack::start();
	}
	else
		Stack::stop();

	Window *active = WinList::get().getActive();
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

void Input::handleMouseMessage(esc::WinMng &winmng,esc::UIEvents::Event *data) {
	const esc::Screen::Mode &mode = WinList::get().getMode();
	gui::Pos old = _cur;
	bool btnChanged = false;
	Window *w;
	Window *wheelWin = NULL;

	_cur = gui::Pos(
		esc::Util::max(0,esc::Util::min((gpos_t)mode.width - 1,_cur.x + data->d.mouse.x)),
		esc::Util::max(0,esc::Util::min((gpos_t)mode.height - 1,_cur.y - data->d.mouse.y))
	);

	/* set active window */
	if(data->d.mouse.buttons != _buttons) {
		btnChanged = true;
		_buttons = data->d.mouse.buttons;
		if(_buttons) {
			w = WinList::get().getAt(_cur);
			if(w) {
				/* do that via message passing so that only one thread performs changes on the
				 * windows */
				winmng.setActive(w->id);
			}
			_mouseWin = w;
		}
	}
	else if(data->d.mouse.z)
		wheelWin = WinList::get().getAt(_cur);

	/* if no buttons are pressed, change the cursor if we're at a window-border */
	if(!_buttons) {
		w = _mouseWin ? _mouseWin : WinList::get().getAt(_cur);
		_cursor = esc::Screen::CURSOR_DEFAULT;
		if(w && w->style != Window::STYLE_POPUP && w->style != Window::STYLE_DESKTOP) {
			gsize_t tbh = w->titleBarHeight;
			bool left = _cur.y >= (gpos_t)(w->y() + tbh) &&
					_cur.x < (gpos_t)(w->x() + esc::Screen::CURSOR_RESIZE_WIDTH);
			bool right = _cur.y >= (gpos_t)(w->y() + tbh) &&
					_cur.x >= (gpos_t)(w->x() + w->width() - esc::Screen::CURSOR_RESIZE_WIDTH);
			bool bottom = _cur.y >= (gpos_t)(w->y() + w->height() - esc::Screen::CURSOR_RESIZE_WIDTH);
			if(left && bottom)
				_cursor = esc::Screen::CURSOR_RESIZE_BL;
			else if(left)
				_cursor = esc::Screen::CURSOR_RESIZE_L;
			if(right && bottom)
				_cursor = esc::Screen::CURSOR_RESIZE_BR;
			else if(right)
				_cursor = esc::Screen::CURSOR_RESIZE_R;
			else if(bottom && !left)
				_cursor = esc::Screen::CURSOR_RESIZE_VERT;
		}
	}

	/* let vesa draw the cursor */
	if(_cur != old)
		WinList::get().setCursor(_cur,_cursor);

	/* send to window */
	w = wheelWin ? wheelWin : (_mouseWin ? _mouseWin : WinList::get().getActive());
	if(w && w->evfd != -1) {
		esc::WinMngEvents::Event ev;
		ev.type = esc::WinMngEvents::Event::TYPE_MOUSE;
		ev.wid = w->id;
		ev.d.mouse.x = _cur.x;
		ev.d.mouse.y = _cur.y;
		ev.d.mouse.movedX = data->d.mouse.x;
		ev.d.mouse.movedY = -data->d.mouse.y;
		ev.d.mouse.movedZ = data->d.mouse.z;
		ev.d.mouse.buttons = data->d.mouse.buttons;
		send(w->evfd,MSG_WIN_EVENT,&ev,sizeof(ev));
	}

	if(btnChanged && !_buttons)
		_mouseWin = NULL;
}
