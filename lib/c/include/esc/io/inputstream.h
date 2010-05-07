/**
 * $Id: inputstream.h 646 2010-05-04 15:19:36Z nasmussen $
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

#ifndef INPUTSTREAM_H_
#define INPUTSTREAM_H_

#include <esc/common.h>
#include <esc/util/string.h>
#include <stdarg.h>

typedef struct sIStream sIStream;

/* abstract */ struct sIStream {
/* private: */
	void *_obj;

/* public: */
	/**
	 * Reads <count> bytes into <buffer> from the given stream.
	 *
	 * @param s the stream
	 * @param buffer the buffer to write to
	 * @param count the number of bytes to read
	 * @return the number of read bytes
	 */
	s32 (*read)(sIStream *s,void *buffer,u32 count);

	/**
	 * Returns the first character in the stream without consuming it.
	 *
	 * @param s the stream
	 * @return the character
	 */
	char (*getc)(sIStream *s);

	/**
	 * Reads a character from the stream
	 *
	 * @param s the stream
	 * @return the character
	 */
	char (*readc)(sIStream *s);

	/**
	 * Reads a string from the stream (i.e. until EOF or until <size> is reached)
	 *
	 * @param s the stream
	 * @param str the string to write to
	 * @param size the number of bytes to read at max.
	 * @return the number of read bytes
	 */
	s32 (*reads)(sIStream *s,char *str,u32 size);

	/**
	 * Reads the whole stream into the given dynamic string
	 *
	 * @param s the stream
	 * @param str the string to write to
	 * @return the number of read bytes
	 */
	s32 (*readAll)(sIStream *s,sString *str);

	/**
	 * Reads an escape-code from the given stream. Assumes that the last read char was '\033'.
	 *
	 * @param s the stream
	 * @param n1 will be set to the first argument (ESCC_ARG_UNUSED if unused)
	 * @param n2 will be set to the second argument (ESCC_ARG_UNUSED if unused)
	 * @param n3 will be set to the third argument (ESCC_ARG_UNUSED if unused)
	 * @return the scanned escape-code (ESCC_*)
	 */
	s32 (*readEsc)(sIStream *s,s32 *n1,s32 *n2,s32 *n3);

	/**
	 * Reads a line from the stream (i.e. until \n, EOF or if <size> is reached)
	 *
	 * @param s the stream
	 * @param str the string to write to
	 * @param size the number of bytes to read at max.
	 * @return the number of read bytes
	 */
	s32 (*readline)(sIStream *s,char *str,u32 size);

	/**
	 * Reads a formated string according to <fmt> from the stream (like scanf)
	 *
	 * @param s the stream
	 * @param fmt the format
	 * @return the number of read items
	 */
	s32 (*readf)(sIStream *s,const char *fmt,...);

	/**
	 * Like readf(), but with given variable-argument-list
	 *
	 * @param s the stream
	 * @param fmt the format
	 * @param ap the argument-list
	 * @return the number of read items
	 */
	s32 (*vreadf)(sIStream *s,const char *fmt,va_list ap);

	/**
	 * 'Unreads' the given character, i.e. puts it back to the stream.
	 *
	 * @param s the stream
	 * @param c the character
	 */
	void (*unread)(sIStream *s,char c);

	/**
	 * @param s the stream
	 * @return the file-descriptor used for this stream
	 */
	tFD (*fileno)(sIStream *s);

	/**
	 * Seeks in the stream
	 *
	 * @param s the stream
	 * @param offset the offset to seek to
	 * @param whence the type of seek: SEEK_CUR, SEEK_SET or SEEK_END
	 * @return the new position
	 */
	s32 (*seek)(sIStream *s,s32 offset,u32 whence);

	/**
	 * Checks wether the stream is at EOF
	 *
	 * @param s the stream
	 * @return true if so
	 */
	bool (*eof)(sIStream *s);

	/**
	 * Closes the stream, i.e. closes the file and frees all memory
	 *
	 * @param s the stream
	 */
	void (*close)(sIStream *s);
};

/**
 * Protected: Just for stream-implementations!
 *
 * @return the stream
 */
sIStream *istream_open(void);

/**
 * Protected: Just for stream-implementations!
 *
 * @param s the stream
 */
void istream_close(sIStream *s);

#endif /* INPUTSTREAM_H_ */
