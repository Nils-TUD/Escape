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

#include <info/cpu.h>
#include <fstream>
#include <assert.h>

using namespace std;

namespace info {
	std::vector<cpu*> cpu::get_list() {
		std::vector<cpu*> list;
		ifstream f("/sys/cpu");
		while(f.good()) {
			cpu *c = new cpu;
			f >> *c;
			list.push_back(c);
		}
		return list;
	}

	istream& operator >>(istream& is,cpu& ci) {
		istream::size_type unlimited = numeric_limits<streamsize>::max();
		is.ignore(unlimited,' ') >> ci._id;
		is.ignore(unlimited,' ') >> ci._total;
		is.ignore(unlimited,'\n');
		is.ignore(unlimited,' ') >> ci._used;
		is.ignore(unlimited,'\n');
		is.ignore(unlimited,' ') >> ci._speed;
		is.ignore(unlimited,'\n');
		while(is.peek() == '\t')
			is.ignore(unlimited,'\n');
		return is;
	}

	std::ostream& operator <<(std::ostream& os,const cpu& ci) {
		os << "CPU[id=" << ci.id() << ", total=" << ci.totalCycles() << ", used=" << ci.usedCycles() << "]";
		return os;
	}
}
