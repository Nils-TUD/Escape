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
	/**
	 * Uses a string as buffer to read from and write to
	 */
	template<class charT,class traits = char_traits<charT> >
	class basic_stringbuf: public basic_streambuf<charT,traits> {
	public:
		typedef typename basic_streambuf<charT,traits>::char_type char_type;
		typedef typename basic_streambuf<charT,traits>::pos_type pos_type;

		/**
		 * Creates a new stringbuffer with given openmode
		 *
		 * @param which the open-mode ((in | out) by default)
		 */
		explicit basic_stringbuf(ios_base::openmode which = ios_base::in | ios_base::out);
		/**
		 * Creates a new stringbuffer with given string and openmode
		 *
		 * @param str the string (will be cloned)
		 * @param which the open-mode ((in | out) by default)
		 */
		explicit basic_stringbuf(const basic_string<charT>& str,
				ios_base::openmode which = ios_base::in | ios_base::out);
		/**
		 * Destructor
		 */
		virtual ~basic_stringbuf();

		/**
		 * @return a copy of the used string
		 */
		basic_string<charT> str() const;
		/**
		 * Sets the string to use for read/write
		 *
		 * @param s the string (will be cloned)
		 */
		void str(const basic_string<charT>& s);

		/**
		 * @return the char at the current position
		 * @throws eof_reached if EOF is reached
		 * @throws bad_state if reading is not allowed
		 */
		virtual char_type peek() const;
		/**
		 * Moves the position-pointer forward
		 *
		 * @return the char at the current position
		 * @throws eof_reached if EOF is reached
		 * @throws bad_state if reading is not allowed
		 */
		virtual char_type get();
		/**
		 * Moves the position-pointer one step backwards
		 *
		 * @throws bad_state if reading is not allowed or we're at position 0
		 */
		virtual void unget();

		/**
		 * Puts the given character into the buffer
		 *
		 * @param c the character
		 * @throws bad_state if writing is not allowed
		 */
		virtual void put(char_type c);
		/**
		 * Does nothing
		 */
		virtual void flush() throw();

	private:
		pos_type _pos;
		ios_base::openmode _mode;
		basic_string<charT> _str;
	};
}

#include "../../../lib/cpp/src/istreams/basic_stringbuf.cc"

#endif /* BASIC_STRINGBUF_H_ */
