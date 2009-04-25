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

#ifndef RTTI_H_
#define RTTI_H_

#define ADD_CXX_TYPEINFO_HEADER(t) \
	class t : public std::type_info { \
	public: \
		virtual ~t(void); \
		explicit t(const char*); \
	}

namespace std {
	class type_info {
	private:
		const char* tname;

	public:
		virtual ~type_info(void);
		type_info(const type_info&);
		explicit type_info(const char*);
		const char* name(void) const;
		bool operator ==(const type_info&) const;
		bool operator !=(const type_info&) const;
	};
}

namespace __cxxabiv1 {
	ADD_CXX_TYPEINFO_HEADER(__fundamental_type_info);
	ADD_CXX_TYPEINFO_HEADER(__array_type_info);
	ADD_CXX_TYPEINFO_HEADER(__function_type_info);
	ADD_CXX_TYPEINFO_HEADER(__enum_type_info);
	ADD_CXX_TYPEINFO_HEADER(__pbase_type_info);
	ADD_CXX_TYPEINFO_HEADER(__pointer_type_info);
	ADD_CXX_TYPEINFO_HEADER(__pointer_to_member_type_info);
	ADD_CXX_TYPEINFO_HEADER(__class_type_info);
	ADD_CXX_TYPEINFO_HEADER(__si_class_type_info);
	ADD_CXX_TYPEINFO_HEADER(__vmi_class_type_info);
}

#undef ADD_CXX_TYPEINFO_HEADER

#endif /* RTTI_H_ */
