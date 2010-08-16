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

#ifndef RAWFILE_H_
#define RAWFILE_H_

#include <stddef.h>
#include <stdio.h>
#include <string>
#include <stdexcept>

namespace std {
	/**
	 * A class to use the "raw"-IO-functions open,seek,read,write and close in a exception-safe way.
	 * I.e. you can allocate the instance of it on the stack, so that, if an exception is thrown,
	 * the destructor is automatically called and closes the file.
	 */
	class rawfile {
	public:
		typedef size_t size_type;
		typedef long off_type;
		typedef int seek_type;
		typedef int open_type;

	public:
		static const open_type READ		= 1;
		static const open_type WRITE	= 2;
		static const open_type APPEND	= 4;
		static const open_type TRUNCATE	= 8;
		static const seek_type SET		= SEEK_SET;
		static const seek_type CUR		= SEEK_CUR;
		static const seek_type END		= SEEK_END;

	public:
		/**
		 * Does nothing
		 */
		rawfile();
		/**
		 * Uses the given file-descriptor (assumes read- and write-support)
		 *
		 * @param fd the file-descriptor
		 */
		rawfile(tFD fd);
		/**
		 * Opens the given file with given mode
		 *
		 * @param filename the filename
		 * @param mode the mode (READ | WRITE | APPEND)
		 * @throws io_exception if it goes wrong
		 */
		rawfile(const string& filename,open_type mode);
		/**
		 * Closes the file, if still open
		 */
		~rawfile();

		/**
		 * Opens the given file with given mode
		 *
		 * @param filename the filename
		 * @param mode the mode (READ | WRITE | APPEND)
		 * @throws io_exception if it goes wrong
		 */
		void open(const string& filename,open_type mode);
		/**
		 * Uses the given file-descriptor. Assumes it to be valid
		 *
		 * @param fd the file-descriptor
		 */
		void use(tFD fd);
		/**
		 * Calls ::seek(fd,<offset>,<whence>)
		 *
		 * @param offset the offset to seek to
		 * @param whence the point where to start (SET | CUR | END)
		 * @throws io_exception if it goes wrong
		 */
		void seek(off_type offset,seek_type whence);
		/**
		 * Calls ::read(fd,<data>,<size> * <count>)
		 *
		 * @param data the buffer to read to
		 * @param size the size of each element
		 * @param count the number of elements to read
		 * @return the number of elements read
		 * @throws io_exception if it goes wrong
		 */
		size_type read(void *data,size_type size,size_type count);
		/**
		 * Calls ::write(fd,<data>,<size> * <count>)
		 *
		 * @param data the buffer to write from
		 * @param size the size of each element
		 * @param count the number of elements to write
		 * @return the number of elements written
		 * @throws io_exception if it goes wrong
		 */
		size_type write(const void *data,size_type size,size_type count);
		/**
		 * Closes the file
		 */
		void close();

	private:
		// no cloning
		rawfile(const rawfile& f);
		rawfile& operator =(const rawfile& f);

	private:
		open_type _mode;
		tFD _fd;
	};
}

#endif /* RAWFILE_H_ */
