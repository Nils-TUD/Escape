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

#include <esc/common.h>
#include <ipc/proto/ui.h>

namespace ipc {

std::ostream &operator<<(std::ostream &os,const UIEvents::Event &ev) {
	static const char *types[] = {
		"KEYBOARD","MOUSE","ACTIVE","INACTIVE"
	};
	os << types[ev.type];
	switch(ev.type) {
		case UIEvents::Event::TYPE_KEYBOARD:
			os << " [" << ev.d.keyb.keycode << "," << ev.d.keyb.modifier << "," << ev.d.keyb.character << "]";
			break;
		case UIEvents::Event::TYPE_MOUSE:
			os << " [" << ev.d.mouse.x << "," << ev.d.mouse.y;
			os << " btns=" << std::hex << ev.d.mouse.buttons << std::dec << "]";
			break;
		default:
			break;
	}
	return os;
}

}
