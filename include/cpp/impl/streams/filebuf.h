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

#ifndef BASIC_FILEBUF_H_
#define BASIC_FILEBUF_H_

#include <stddef.h>
#include <esc/io.h>
#include <impl/streams/ios_base.h>
#include <streambuf>

namespace std {
	/**
	 * Read/write from/to a file
	 */
	class filebuf: public streambuf {
	private:
		static const pos_type IN_BUF_SIZE = 512;
		static const pos_type OUT_BUF_SIZE = 512;

	public:
		typedef streambuf::char_type char_type;
		typedef streambuf::pos_type pos_type;

		/**
		 * Creates a new filebuf
		 */
		explicit filebuf()
			: _fd(-1), _inPos(0), _inMax(0), _inBuf(NULL), _totalInPos(0), _outPos(0),
			  _outBuf(NULL), _mode(0) {
		}
		/**
		 * Destructor
		 */
		virtual ~filebuf() {
			close();
		}

		/**
		 * Opens the file <s> with given open-mode
		 *
		 * @param s the file
		 * @param mode the mode
		 * @return this on success, a null-pointer on failure
		 */
		filebuf* open(const char* s,ios_base::openmode mode);

		/**
		 * Uses the given file-descriptor with given open-mode
		 *
		 * @param fd the file-descriptor
		 * @param mode the mode
		 * @return this on success, a null-pointer on failure
		 */
		filebuf* open(tFD fd,ios_base::openmode mode);

		/**
		 * @return if a file has been opened successfully
		 */
		bool is_open() const {
			return _fd >= 0;
		}
		/**
		 * @return the file-descriptor
		 */
		int fd() const {
			return _fd;
		}

		/**
		 * Closes the file
		 * @return this on success, a null-pointer on failure
		 */
		filebuf* close();

		/**
		 * @return the number of available characters
		 */
		virtual pos_type available() const;

		/**
		 * @return the char at the current position (or EOF)
		 * @throws bad_state if reading is not allowed
		 */
		virtual char_type peek() const;
		/**
		 * Moves the position-pointer forward
		 *
		 * @return the char at the current position (or EOF)
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
		 * Writes all pending output to file
		 */
		virtual void flush();

	private:
		unsigned char getMode(ios_base::openmode mode);
		bool fillBuffer() const;

	private:
		tFD _fd;
		mutable pos_type _inPos;
		mutable pos_type _inMax;
		mutable char* _inBuf;
		pos_type _totalInPos;
		pos_type _outPos;
		char* _outBuf;
		ios_base::openmode _mode;
	};
}

#endif /* BASIC_FILEBUF_H_ */
