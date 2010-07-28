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

#ifndef BASIC_STRINGSTREAM_H_
#define BASIC_STRINGSTREAM_H_

#include <istreams/basic_iostream.h>

namespace std {
	template<class charT,class traits = char_traits<charT> >
	class basic_stringstream: public basic_iostream<charT,traits> {
	public:
		explicit basic_stringstream(basic_string<charT>& str,
				ios_base::openmode which = ios_base::out | ios_base::in);
		virtual ~basic_stringstream();

		basic_stringbuf<charT,traits>* rdbuf() const;
		basic_string<charT>& str() const;
		void str(basic_string<charT>& s);
	};
}

#include "../../../lib/cpp/src/istreams/basic_stringstream.cc"

#endif /* BASIC_STRINGSTREAM_H_ */
