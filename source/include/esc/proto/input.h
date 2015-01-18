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

#include <esc/proto/default.h>
#include <sys/common.h>
#include <sys/messages.h>

namespace esc {

/**
 * The keyboard-event sent by the keyboard-device
 */
struct Keyb {
	struct Event {
		static const msgid_t MID = MSG_KB_EVENT;

		enum {
			FL_BREAK	= 1 << 0,
			FL_CAPS		= 1 << 1,
		};

		/* the keycode (see keycodes.h) */
		uchar keycode;
		uchar flags;
	};
};

/**
 * The mouse-event sent by the mouse-device
 */
struct Mouse {
	struct Event {
		static const msgid_t MID = MSG_MS_EVENT;

		gpos_t x;
		gpos_t y;
		gpos_t z;
		uchar buttons;
	};
};

}
