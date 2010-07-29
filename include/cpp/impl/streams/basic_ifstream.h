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

#ifndef BASIC_IFSTREAM_H_
#define BASIC_IFSTREAM_H_

#include <impl/streams/basic_filebuf.h>
#include <impl/streams/basic_istream.h>

namespace std {
	/**
	 * A file-stream for input-operations
	 */
	template<class charT,class traits = char_traits<charT> >
	class basic_ifstream: public basic_istream<charT,traits> {
	public:
		/**
		 * Builds a new file-stream
		 */
		basic_ifstream();
		/**
		 * Builds a new file-stream and opens the given file
		 *
		 * @param filename the file to open
		 * @param which the openmode (in by default)
		 */
		explicit basic_ifstream(const char* filename,ios_base::openmode which = ios_base::in);
		/**
		 * Destructor
		 */
		virtual ~basic_ifstream();

		/**
		 * @return the file-buffer
		 */
		basic_filebuf<charT,traits>* rdbuf() const;
		/**
		 * Opens the file <s> with given open-mode
		 *
		 * @param s the file
		 * @param mode the mode
		 */
		void open(const char* s,ios_base::openmode mode = ios_base::in);
		/**
		 * @return if a file has been opened successfully
		 */
		bool is_open() const;
		/**
		 * Closes the file
		 */
		void close();
	};
}

#include "../../../../lib/cpp/src/impl/streams/basic_ifstream.cc"

#endif /* BASIC_IFSTREAM_H_ */
