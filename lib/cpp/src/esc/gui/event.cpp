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
#include <esc/gui/common.h>
#include <esc/gui/event.h>
#include <esc/stream.h>

namespace esc {
	namespace gui {
		Stream &operator<<(Stream &s,const MouseEvent &e) {
			s << "MouseEvent[mx=" << e._movedx << ",my=" << e._movedy;
			s << ",x=" << e._x << ",y=" << e._y;
			s.format(",buttons=%#x]",e._buttons);
			return s;
		}

		Stream &operator<<(Stream &s,const KeyEvent &e) {
			s << "KeyEvent[keycode=" << e._keycode << ",char=" << e._character;
			s.format(",modifier=%#x]",e._modifier);
			return s;
		}
	}
}
