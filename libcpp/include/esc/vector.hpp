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

#ifndef VECTOR_
#define VECTOR_

#include <types.hpp>

namespace esc {
	template<class T,int initialSize,int increment>
	class vector {
	private:
		u32 x;
		size_type size;
		T* elements[initialSize];

	public:
		vector() : size(0) {

		};
		~vector() {

		};

		inline size_type size() const {
			return size;
		};
	};
}

#endif /* VECTOR_ */
