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

#ifndef BASIC_OFSTREAM_H_
#define BASIC_OFSTREAM_H_

#include <impl/streams/filebuf.h>
#include <ostream>
#include <esc/io.h>

namespace esc {
	/**
	 * A file-stream for input-operations
	 */
	class ofstream: public ostream {
	public:
		/**
		 * Builds a new file-stream
		 */
		ofstream();
		/**
		 * Builds a new file-stream and opens the given file
		 *
		 * @param filename the file to open
		 * @param which the openmode (out by default)
		 */
		explicit ofstream(const char* filename,ios_base::openmode which = ios_base::out);
		/**
		 * Destructor
		 */
		virtual ~ofstream();

		/**
		 * @return the file-buffer
		 */
		filebuf* rdbuf() const;
		/**
		 * Uses the given file-descriptor
		 *
		 * @param fd the file-descriptor
		 * @param which the open-mode (out by default)
		 */
		void open(tFD fd,ios_base::openmode which = ios_base::out);
		/**
		 * Opens the file <s> with given open-mode
		 *
		 * @param s the file
		 * @param mode the mode
		 */
		void open(const char* s,ios_base::openmode mode = ios_base::out);
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

#endif /* BASIC_OFSTREAM_H_ */
