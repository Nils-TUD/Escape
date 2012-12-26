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

#include <info/memusage.h>
#include <fstream>
#include <assert.h>

namespace info {
	memusage memusage::get() {
		memusage mem;
		ifstream f("/system/memusage");
		f >> mem;
		return mem;
	}

	std::istream& operator >>(std::istream& is,memusage& mem) {
		istream::size_type unlimited = numeric_limits<streamsize>::max();
		is.ignore(unlimited,' ') >> mem._total;
		is.ignore(unlimited,' ') >> mem._used >> std::ws;
		is.ignore(unlimited,'\n');
		is.ignore(unlimited,' ') >> mem._swapTotal;
		is.ignore(unlimited,' ') >> mem._swapUsed;
		return is;
	}

	std::ostream& operator <<(std::ostream& os,const memusage& mem) {
		os << "memusage[total=" << mem.total() << ", used=" << mem.used() << ",";
		os << " swapTotal=" << mem.swapTotal() << ", swapUsed=" << mem.swapUsed() << "]";
		return os;
	}
}
