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

namespace std {
	// this is mostly inspired by the stuff from gcc's libstdc++.

	template<typename Base,typename Derived>
	struct is_base_of {
		static const bool value = __is_base_of(Base,Derived);
	};

	// convertible
	template<class From,class To>
	struct is_convertible {
		void check() {
			To y A_UNUSED = _x;
		}
		From _x;
	};
	template<class From,class To>
	struct is_const_convertible {
		void check() {
			To y A_UNUSED = const_cast<To>(_x);
		}
		From _x;
	};
	template<class From,class To>
	struct is_static_convertible {
		void check() {
			To y A_UNUSED = static_cast<To>(_x);
		}
		From _x;
	};

	template<class Concept>
	inline void function_requires() {
		void (Concept::*_x)() A_UNUSED = &Concept::check;
	}
}
