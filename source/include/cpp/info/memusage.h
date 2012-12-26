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

#include <esc/common.h>
#include <istream>

namespace info {
	class memusage;
	std::istream& operator >>(std::istream& is,memusage& mem);
	std::ostream& operator <<(std::ostream& os,const memusage& mem);

	class memusage {
		friend std::istream& operator >>(std::istream& is,memusage& mem);
	public:
		typedef size_t size_type;

		static memusage get();

		explicit memusage() : _total(), _used(), _swapTotal(), _swapUsed() {
		}

		size_type total() const {
			return _total;
		}
		size_type used() const {
			return _used;
		}
		size_type swapTotal() const {
			return _swapTotal;
		}
		size_type swapUsed() const {
			return _swapUsed;
		}

	private:
		size_type _total;
		size_type _used;
		size_type _swapTotal;
		size_type _swapUsed;
	};
}
