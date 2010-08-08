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

#include <impl/streams/stringbuf.h>
#include <iostream>
#include <string>

namespace std {
	/**
	 * A string-stream for input- and output-operations
	 */
	class stringstream: public iostream {
	public:
		/**
		 * Builds a new string-stream with given openmode
		 *
		 * @param which the openmode (in|out by default)
		 */
		explicit stringstream(ios_base::openmode which = ios_base::out | ios_base::in);
		/**
		 * Builds a new string-stream with given string and openmode
		 *
		 * @param str the string (makes a clone)
		 * @param which the openmode (in|out by default)
		 */
		explicit stringstream(const string& str,
				ios_base::openmode which = ios_base::out | ios_base::in);
		/**
		 * Destructor
		 */
		virtual ~stringstream();

		/**
		 * @return the string-buffer
		 */
		stringbuf* rdbuf() const;
		/**
		 * @return a copy of the string used by the string-buffer
		 */
		string str() const;
		/**
		 * Sets the string used by the string-buffer
		 *
		 * @param s the string (makes a clone)
		 */
		void str(const string& s);
	};
}

#endif /* BASIC_STRINGSTREAM_H_ */
