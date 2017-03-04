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

#include <esc/proto/initui.h>
#include <esc/stream/ostream.h>
#include <esc/stream/istream.h>
#include <map>

namespace info {
	class ui;
	esc::IStream& operator >>(esc::IStream& is,ui& u);
	esc::OStream& operator <<(esc::OStream& os,const ui& u);

	class ui {
		friend esc::IStream& operator >>(esc::IStream& is,ui& u);
	public:
		typedef size_t id_type;

		static std::map<id_type,ui> get_list();

		explicit ui() : _id(), _type(), _mode() {
		}

		id_type id() const {
			return _id;
		}
		esc::InitUI::Type type() const {
			return _type;
		}
		int mode() const {
			return _mode;
		}

	private:
		id_type _id;
		esc::InitUI::Type _type;
		int _mode;
	};
}
