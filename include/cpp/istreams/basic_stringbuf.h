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

#ifndef BASIC_STRINGBUF_H_
#define BASIC_STRINGBUF_H_

#include <stddef.h>
#include <istreams/basic_streambuf.h>

namespace std {
	template<class charT,class traits = char_traits<charT> >
	class basic_stringbuf: public basic_streambuf<charT,traits> {
	public:
		typedef typename basic_streambuf<charT,traits>::char_type char_type;
		typedef typename basic_streambuf<charT,traits>::pos_type pos_type;

		explicit basic_stringbuf(string& str,ios_base::openmode which = ios_base::in | ios_base::out);
		virtual ~basic_stringbuf();

		virtual char_type peek() const;
		virtual char_type get();
		virtual bool unget();

		virtual bool put(char_type c);

	private:
		pos_type _pos;
		ios_base::openmode _mode;
		string &_str;
	};
}

#include "../../../lib/cpp/src/istreams/basic_stringbuf.cc"

#endif /* BASIC_STRINGBUF_H_ */
