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

#ifndef BASIC_ISTRINGSTREAM_H_
#define BASIC_ISTRINGSTREAM_H_

#include <istreams/basic_istream.h>

namespace std {
	/**
	 * A string-stream for input-operations
	 */
	template<class charT,class traits = char_traits<charT> >
	class basic_istringstream: public basic_istream<charT,traits> {
	public:
		/**
		 * Builds a new input-string-stream with given openmode
		 *
		 * @param which the openmode (in by default)
		 */
		explicit basic_istringstream(ios_base::openmode which = ios_base::in);
		/**
		 * Builds a new input-string-stream with given string and openmode
		 *
		 * @param str the string (will be cloned)
		 * @param which the openmode (in by default)
		 */
		explicit basic_istringstream(const basic_string<charT>& str,
				ios_base::openmode which = ios_base::in);
		/**
		 * Destructor
		 */
		virtual ~basic_istringstream();

		/**
		 * @return the string-buffer
		 */
		basic_stringbuf<charT,traits>* rdbuf() const;
		/**
		 * @return a copy of the string used by the string-buffer
		 */
		basic_string<charT> str() const;
		/**
		 * Sets the string used by the string-buffer
		 *
		 * @param s the string (will be cloned)
		 */
		void str(const basic_string<charT>& s);
	};
}

#include "../../../lib/cpp/src/istreams/basic_istringstream.cc"

#endif /* BASIC_ISTRINGSTREAM_H_ */
