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

#ifndef OUTPUTSTREAM_H_
#define OUTPUTSTREAM_H_

#include <esc/common.h>
#include <stdarg.h>

typedef struct sOStream sOStream;

/* abstract */ struct sOStream {
/* private: */
	void *_obj;

/* public: */

	/**
	 * Writes <count> bytes from <buffer> into the stream
	 *
	 * @param s the stream
	 * @param buffer the buffer to write
	 * @param count the number of bytes
	 * @return the number of written bytes
	 */
	s32 (*write)(sOStream *s,const void *buffer,u32 count);

	/**
	 * Writes the given character into the stream
	 *
	 * @param s the stream
	 * @param c the character
	 * @return the number of written bytes
	 */
	s32 (*writec)(sOStream *s,char c);

	/**
	 * Writes the given string into the stream
	 *
	 * @param s the stream
	 * @param str the string
	 * @return the number of written bytes
	 */
	s32 (*writes)(sOStream *s,const char *str);

	/**
	 * Writes the given string into the string, followed by a '\n'
	 *
	 * @param s the stream
	 * @param str the string
	 * @return the number of written bytes
	 */
	s32 (*writeln)(sOStream *s,const char *str);

	/**
	 * Converts the given integer to a string and writes it into the stream
	 *
	 * @param s the stream
	 * @param n the number
	 * @return the number of written bytes
	 */
	s32 (*writen)(sOStream *s,s32 n);

	/**
	 * Converts the given long integer to a string and writes it into the stream
	 *
	 * @param s the stream
	 * @param n the number
	 * @return the number of written bytes
	 */
	s32 (*writel)(sOStream *s,s64 n);

	/**
	 * Converts the given integer to a string in the given base and writes it into the stream
	 *
	 * @param s the stream
	 * @param u the number
	 * @param base the base (2..16)
	 * @return the number of written bytes
	 */
	s32 (*writeu)(sOStream *s,u32 u,u8 base);

	/**
	 * Converts the given long integer to a string in the given base and writes it into the stream
	 *
	 * @param s the stream
	 * @param u the number
	 * @param base the base (2..16)
	 * @return the number of written bytes
	 */
	s32 (*writeul)(sOStream *s,u64 u,u8 base);

	/**
	 * Converts the given double to string with given precision and writes it into the stream
	 *
	 * @param s the stream
	 * @param d the double
	 * @param precision the number of fraction-digits
	 * @return the number of written bytes
	 */
	s32 (*writedbl)(sOStream *s,double d,u32 precision);

	/**
	 * Writes formated output according to <fmt> into the stream (like printf)
	 *
	 * @param s the stream
	 * @param fmt the format
	 * @return the number of written bytes
	 */
	s32 (*writef)(sOStream *s,const char *fmt,...);

	/**
	 * Like writef(), but with given variable-argument-list
	 *
	 * @param s the stream
	 * @param fmt the format
	 * @param ap the argument-list
	 * @return the number of written bytes
	 */
	s32 (*vwritef)(sOStream *s,const char *fmt,va_list ap);

	/**
	 * Seeks in the stream
	 *
	 * @param s the stream
	 * @param offset the offset to seek to
	 * @param whence the type of seek: SEEK_CUR, SEEK_SET or SEEK_END
	 * @return the new position
	 */
	s32 (*seek)(sOStream *s,s32 offset,u32 whence);

	/**
	 * Checks wether the stream is at EOF
	 *
	 * @param s the stream
	 * @return true if so
	 */
	bool (*eof)(sOStream *s);

	/**
	 * Flushes all buffered output
	 *
	 * @param s the stream
	 */
	void (*flush)(sOStream *s);

	/**
	 * Closes the stream, i.e. flushes all output, closes the file and frees all memory
	 *
	 * @param s the stream
	 */
	void (*close)(sOStream *s);
};

/**
 * Protected: Just for stream-implementations!
 *
 * @return the stream
 */
sOStream *ostream_open(void);

/**
 * Protected: Just for stream-implementations!
 *
 * @param s the stream
 */
void ostream_close(sOStream *s);

#endif /* OUTPUTSTREAM_H_ */
