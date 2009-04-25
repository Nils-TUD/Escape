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

#include <rtti.h>

#define ADD_CXX_TYPEINFO_SOURCE(t) \
      t::~t(void) {} \
      t::t(const char* n) : std::type_info (n) {}

namespace std {
	type_info::~type_info(void) {
	}

	type_info::type_info(const type_info& arg) : tname(arg.tname) {
	}

	type_info::type_info(const char* pname) : tname(pname) {
	}

	const char* type_info::name(void) const {
		return tname;
	}

	bool type_info::operator ==(const type_info& arg) const {
		return tname == arg.tname;
	}

	bool type_info::operator !=(const type_info& arg) const {
		return tname != arg.tname;
	}
}

namespace __cxxabiv1 {
	ADD_CXX_TYPEINFO_SOURCE(__fundamental_type_info)
	ADD_CXX_TYPEINFO_SOURCE(__array_type_info)
	ADD_CXX_TYPEINFO_SOURCE(__function_type_info)
	ADD_CXX_TYPEINFO_SOURCE(__enum_type_info)
	ADD_CXX_TYPEINFO_SOURCE(__pbase_type_info)
	ADD_CXX_TYPEINFO_SOURCE(__pointer_type_info)
	ADD_CXX_TYPEINFO_SOURCE(__pointer_to_member_type_info)
	ADD_CXX_TYPEINFO_SOURCE(__class_type_info)
	ADD_CXX_TYPEINFO_SOURCE(__si_class_type_info)
	ADD_CXX_TYPEINFO_SOURCE(__vmi_class_type_info)
}

#undef ADD_CXX_TYPEINFO_SOURCE
