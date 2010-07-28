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

#ifndef BASIC_IOSTREAM_H_
#define BASIC_IOSTREAM_H_

#include <istreams/basic_istream.h>
#include <istreams/basic_ostream.h>

namespace std {
	template<class charT,class traits = char_traits<charT> >
	class basic_iostream : public basic_istream<charT,traits>, public basic_ostream<charT,traits> {
	public:
		explicit basic_iostream(basic_streambuf<charT,traits>* sb);
		virtual ~basic_iostream();
	};
}

#include "../../../lib/cpp/src/istreams/basic_iostream.cc"

#endif /* BASIC_IOSTREAM_H_ */
