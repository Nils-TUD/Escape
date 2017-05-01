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

	WinList::get().sendKeyEvent(*data);
}

void Input::handleMouseMessage(esc::UIEvents::Event *data) {
	const esc::Screen::Mode &mode = WinList::get().getMode();
	gui::Pos old = _cur;
	bool btnChanged = false;
	gwinid_t wid;
	gwinid_t wheelWin = WINID_UNUSED;

	_cur = gui::Pos(
		esc::Util::max(0,esc::Util::min((gpos_t)mode.width - 1,_cur.x + data->d.mouse.x)),
		esc::Util::max(0,esc::Util::min((gpos_t)mode.height - 1,_cur.y - data->d.mouse.y))
	);

	/* set active window */
	if(data->d.mouse.buttons != _buttons) {
		btnChanged = true;
		_buttons = data->d.mouse.buttons;
		if(_buttons) {
			wid = WinList::get().getAt(_cur);
			if(wid != WINID_UNUSED)
				WinList::get().setActive(wid,true,true);
			_mouseWin = wid;
		}
	}
	else if(data->d.mouse.z)
		wheelWin = WinList::get().getAt(_cur);

	/* if no buttons are pressed, change the cursor if we're at a window-border */
	if(!_buttons) {
		wid = _mouseWin != WINID_UNUSED ? _mouseWin : WinList::get().getAt(_cur);
		_cursor = esc::Screen::CURSOR_DEFAULT;

		gui::Rectangle r;
		gsize_t tbh;
		uint style;
		if(WinList::get().getInfo(wid,&r,&tbh,&style)) {
			if(style != Window::STYLE_POPUP && style != Window::STYLE_DESKTOP) {
				bool left = _cur.y >= (gpos_t)(r.y() + tbh) &&
						_cur.x < (gpos_t)(r.x() + esc::Screen::CURSOR_RESIZE_WIDTH);
				bool right = _cur.y >= (gpos_t)(r.y() + tbh) &&
						_cur.x >= (gpos_t)(r.x() + r.width() - esc::Screen::CURSOR_RESIZE_WIDTH);
				bool bottom = _cur.y >= (gpos_t)(r.y() + r.height() - esc::Screen::CURSOR_RESIZE_WIDTH);
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
	}

	/* let vesa draw the cursor */
	if(_cur != old)
		WinList::get().setCursor(_cur,_cursor);

	/* send to window */
	wid = wheelWin != WINID_UNUSED ? wheelWin :
			(_mouseWin != WINID_UNUSED ? _mouseWin : WinList::get().getActive());
	WinList::get().sendMouseEvent(wid,_cur,*data);

	if(btnChanged && !_buttons)
		_mouseWin = WINID_UNUSED;
}
