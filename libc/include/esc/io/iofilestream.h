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

#ifndef IOFILESTREAM_H_
#define IOFILESTREAM_H_

#include <esc/common.h>
#include <esc/io/ifilestream.h>
#include <esc/io/ofilestream.h>

typedef struct sIOStream sIOStream;

struct sIOStream {
/* private: */
	s32 _error;

/* public: */
	sIStream *in;
	sOStream *out;

	/**
	 * Closes this stream
	 *
	 * @param s the stream
	 */
	void (*close)(sIOStream *s);
};

/**
 * Opens a stream for reading XOR writing. This is mainly intended for the stdio-implementation
 * since we just have a FILE* and may be reading or writing with it.
 *
 * @param path the path to open (has not to be absolute)
 * @param mode the mode (IO_*)
 * @return the stream
 */
sIOStream *iofstream_open(const char *path,u8 mode);

/**
 * Like iofstream_open(), but uses the file denoted by the given file-descriptor.
 *
 * @param fd the file-descriptor
 * @param mode the mode (IO_*)
 * @return the stream
 */
sIOStream *iofstream_openfd(tFD fd,u8 mode);

/**
 * Links the given input-stream to an IO-stream
 *
 * @param out the input-stream
 * @return the stream
 */
sIOStream *iofstream_linkin(sIStream *in);

/**
 * Links the given output-stream to an IO-stream
 *
 * @param out the output-stream
 * @return the stream
 */
sIOStream *iofstream_linkout(sOStream *out);

#endif /* IOFILESTREAM_H_ */
