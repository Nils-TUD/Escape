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

#include <impl/streams/stringbuf.h>
#include <ostream>
#include <string>

namespace std {
	/**
	 * A string-stream for output-operations
	 */
	class ostringstream: public ostream {
	public:
		/**
		 * Builds a new output-string-stream with given openmode
		 *
		 * @param which the openmode (out by default)
		 */
		explicit ostringstream(ios_base::openmode which = ios_base::out)
			: ostream(new stringbuf(which)) {
		}
		/**
		 * Builds a new output-string-stream with given string and openmode
		 *
		 * @param str the string (will be cloned)
		 * @param which the openmode (out by default)
		 */
		explicit ostringstream(const string& s,ios_base::openmode which = ios_base::out)
			: ostream(new stringbuf(s,which)) {
		}
		/**
		 * Destructor
		 */
		virtual ~ostringstream() {
		}

		/**
		 * @return the string-buffer
		 */
		stringbuf* rdbuf() const {
			return static_cast<stringbuf*>(ios::rdbuf());
		}
		/**
		 * @return a copy of the string used by the string-buffer
		 */
		string str() const {
			return rdbuf()->str();
		}
		/**
		 * Sets the string used by the string-buffer
		 *
		 * @param s the string (will be cloned)
		 */
		void str(const string& s) {
			rdbuf()->str(s);
		}
	};
}
