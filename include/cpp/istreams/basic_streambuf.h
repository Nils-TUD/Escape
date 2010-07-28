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

#ifndef BASIC_STREAMBUF_H_
#define BASIC_STREAMBUF_H_

#include <stddef.h>

namespace std {
	/**
	 * The exception that should be thrown if a buffer is at EOF
	 */
	class eof_reached : public exception {
	public:
		explicit eof_reached();
		virtual const char* what() const throw();
	};

	/**
	 * The exception that should be thrown if a buffer is in an invalid state for an operation
	 */
	class bad_state : public exception {
	public:
		explicit bad_state(const string & msg);
		virtual const char* what() const throw();
	private:
		const char *_msg;
	};

	/**
	 * The base-class for all stream-buffers
	 */
	template<class charT,class traits = char_traits<charT> >
	class basic_streambuf {
	public:
		typedef charT char_type;
		typedef typename traits::int_type int_type;
		typedef typename traits::pos_type pos_type;
		typedef typename traits::off_type off_type;
		typedef traits traits_type;

		/**
		 * Creates a stream-buffer (does nothing)
		 */
		basic_streambuf();
		/**
		 * Destructor (does nothing)
		 */
		virtual ~basic_streambuf();

		/**
		 * @return the char at the current position
		 */
		virtual char_type peek() const = 0;
		/**
		 * Moves the position-pointer forward.
		 *
		 * @return the char at the current position
		 */
		virtual char_type get() = 0;
		/**
		 * Moves the position-pointer one step backwards
		 */
		virtual void unget() = 0;

		/**
		 * Puts the given character into the buffer
		 *
		 * @param c the character
		 */
		virtual void put(char_type c) = 0;
		/**
		 * Flushes the buffer (whatever that means for a specific buffer-implementation)
		 */
		virtual void flush() = 0;
	};
}

#include "../../../lib/cpp/src/istreams/basic_streambuf.cc"

#endif /* BASIC_STREAMBUF_H_ */
