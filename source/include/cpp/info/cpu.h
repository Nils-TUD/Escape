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

#pragma once

#include <info/process.h>
#include <istream>
#include <vector>

namespace info {
	class cpu;
	std::istream& operator >>(std::istream& is,cpu& ci);
	std::ostream& operator <<(std::ostream& os,const cpu& ci);

	class cpu {
		friend std::istream& operator >>(std::istream& is,cpu& ci);
	public:
		typedef unsigned id_type;
		typedef process::cycle_type cycle_type;

		static std::vector<cpu*> get_list();

		explicit cpu() : _id(), _total(), _used() {
		}

		id_type id() const {
			return _id;
		}
		cycle_type totalCycles() const {
			return _total;
		}
		cycle_type usedCycles() const {
			return _used;
		}

	private:
		id_type _id;
		cycle_type _total;
		cycle_type _used;
	};
}
