/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <stddef.h>
#include <sys/io.h>
#include <impl/streams/ios_base.h>
#include <streambuf>

namespace std {
	/**
	 * Read/write from/to a file
	 */
	class filebuf: public streambuf {
	private:
		static const pos_type IN_BUF_SIZE = 4096;
		static const pos_type OUT_BUF_SIZE = 4096;

	public:
		typedef streambuf::char_type char_type;
		typedef streambuf::pos_type pos_type;

		/**
		 * Creates a new filebuf
		 */
		explicit filebuf()
			: _fd(-1), _close(false), _inPos(0), _inMax(0), _inBuf(nullptr), _totalInPos(0), _outPos(0),
			  _outBuf(nullptr), _mode(0) {
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
		 * Uses the given file-descriptor with given open-mode. It will NOT close the file in the
		 * destructor.
		 *
		 * @param fd the file-descriptor
		 * @param mode the mode
		 * @return this on success, a null-pointer on failure
		 */
		filebuf* open(int fd,ios_base::openmode mode);

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
		 *
		 * @param whether to destroy the stream
		 * @return this on success, a null-pointer on failure
		 */
		filebuf* close(bool destroy = true);

		/**
		 * @return the number of available characters
		 */
		virtual pos_type available() const;
		/**
		 * @return the number of remaining characters in the buffer
		 */
		virtual pos_type remaining() const {
			return _inMax - _inPos;
		}

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
		 * @return a pointer to the buffered input data
		 */
		virtual const char_type *input() const;

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
		int _fd;
		bool _close;
		mutable pos_type _inPos;
		mutable pos_type _inMax;
		mutable char* _inBuf;
		pos_type _totalInPos;
		pos_type _outPos;
		char* _outBuf;
		ios_base::openmode _mode;
	};
}
