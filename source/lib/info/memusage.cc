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
#include <info/memusage.h>
#include <assert.h>

using namespace esc;

namespace info {
	memusage memusage::get() {
		memusage mem;
		FStream f("/sys/memusage","r");
		f >> mem;
		return mem;
	}

	IStream& operator >>(IStream& is,memusage& mem) {
		size_t unlimited = std::numeric_limits<size_t>::max();
		is.ignore(unlimited,' ') >> mem._total;
		is.ignore(unlimited,' ') >> mem._used;
		ulong dummy;
		is.ignore(unlimited,' ') >> dummy;
		is.ignore(unlimited,' ') >> mem._swapTotal;
		is.ignore(unlimited,' ') >> mem._swapUsed;
		return is;
	}

	OStream& operator <<(OStream& os,const memusage& mem) {
		os << "memusage[total=" << mem.total() << ", used=" << mem.used() << ",";
		os << " swapTotal=" << mem.swapTotal() << ", swapUsed=" << mem.swapUsed() << "]";
		return os;
	}
}
