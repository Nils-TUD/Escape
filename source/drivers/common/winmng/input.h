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

#pragma once

#include <esc/proto/ui.h>
#include <esc/proto/winmng.h>
#include <sys/common.h>
#include <sys/thread.h>
#include <stdlib.h>

#include "window.h"

class Input {
	explicit Input(esc::UIEvents *uiev)
		: _buttons(), _cur(), _cursor(esc::Screen::CURSOR_DEFAULT), _mouseWin(WINID_UNUSED), _uiev(uiev) {
		if(startthread(thread,this) < 0)
			error("Unable to start input thread");
	}

public:
	static void create(esc::UIEvents *uiev) {
		_inst = new Input(uiev);
	}
	static Input &get() {
		return *_inst;
	}

	const gui::Pos &getMouse() const {
		return _cur;
	}

private:
	void handleKbMessage(esc::UIEvents::Event *data);
	void handleMouseMessage(esc::UIEvents::Event *data);

	static int thread(void *arg);

	uchar _buttons;
	gui::Pos _cur;
	uchar _cursor;
	gwinid_t _mouseWin;
	esc::UIEvents *_uiev;
	static Input *_inst;
};
