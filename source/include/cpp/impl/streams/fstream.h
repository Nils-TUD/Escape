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

#ifndef BASIC_FSTREAM_H_
#define BASIC_FSTREAM_H_

#include <impl/streams/filebuf.h>
#include <iostream>

namespace std {
	/**
	 * A file-stream for input- and output-operations
	 */
	class fstream: public iostream {
	public:
		/**
		 * Builds a new file-stream
		 */
		fstream()
			: iostream(new filebuf()) {
		}
		/**
		 * Builds a new file-stream and opens the given file
		 *
		 * @param filename the file to open
		 * @param which the openmode (in|out by default)
		 */
		explicit fstream(const char* filename,
				ios_base::openmode which = ios_base::out | ios_base::in)
			: iostream(new filebuf()) {
			open(filename,which);
		}
		/**
		 * Destructor
		 */
		virtual ~fstream() {
			delete rdbuf();
		}

		/**
		 * @return the file-buffer
		 */
		filebuf* rdbuf() const {
			return static_cast<filebuf*>(ios::rdbuf());
		}
		/**
		 * @return the file-descriptor
		 */
		int filedesc() const {
			return rdbuf()->fd();
		}
		/**
		 * Opens the file <s> with given open-mode
		 *
		 * @param s the file
		 * @param mode the mode
		 */
		void open(const char* s,ios_base::openmode mode = ios_base::out | ios_base::in) {
			if(!rdbuf()->open(s,mode))
				setf(failbit);
		}
		/**
		 * @return if a file has been opened successfully
		 */
		bool is_open() const {
			return rdbuf()->is_open();
		}
		/**
		 * Closes the file
		 */
		void close() {
			rdbuf()->close();
		}
	};
}

#endif /* BASIC_FSTREAM_H_ */
