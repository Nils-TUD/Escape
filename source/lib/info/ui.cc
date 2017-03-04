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

#include <esc/stream/fstream.h>
#include <info/ui.h>
#include <assert.h>

using namespace esc;

namespace info {
	std::map<ui::id_type,ui> ui::get_list() {
		std::map<id_type,ui> res;
		FStream f("/sys/uis","r");
		while(f.good()) {
			ui u;
			f >> u;
			if(!f.good())
				break;
			res[u.id()] = u;
		}
		return res;
	}

	IStream& operator >>(IStream& is,ui& u) {
		int type;
		is >> u._id >> type >> u._mode >> u._keymap;
		u._type = static_cast<esc::InitUI::Type>(type);
		return is;
	}

	OStream& operator <<(OStream& os,const ui& u) {
		os << "ARP[id=" << u.id() << ", type=" << u.type()
		   << ", mode=" << u.mode() << ", keymap=" << u.keymap() << "]";
		return os;
	}
}
