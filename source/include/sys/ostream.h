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

#include <sys/common.h>
#include <stdarg.h>

class OStream {
public:
	explicit OStream() : indent(), lineStart(true) {
	}
	virtual ~OStream() {
	}

	/**
	 * Increases the indent-level by 1. The indent is applied after every newline.
	 */
	void pushIndent() {
		indent++;
	}
	/**
	 * Decreases the indent-level by 1.
	 */
	void popIndent() {
		indent--;
	}

	/**
	 * Writes the given arguments according to <fmt> into this stream.
	 *
	 * @param fmt the format
	 */
	void writef(const char *fmt,...) {
		va_list ap;
		va_start(ap,fmt);
		vwritef(fmt,ap);
		va_end(ap);
	}

	/**
	 * Same as writef, but with the va_list as argument
	 *
	 * @param fmt the format
	 * @param ap the argument-list
	 */
	void vwritef(const char *fmt,va_list ap);

	/**
	 * Writes <c> into the stream
	 *
	 * @param c the character
	 */
	virtual void writec(char c) = 0;

private:
	/**
	 * Handles an escape-sequence
	 */
	virtual bool escape(const char **) {
		return false;
	}
	/**
	 * @return the pad-value for '|'
	 */
	virtual uchar pipepad() const {
		return 0;
	}

	void printc(char c);
	void printnpad(llong n,uint pad,uint flags);
	void printupad(ullong u,uint base,uint pad,uint flags);
	int printpad(int count,uint flags);
	int printu(ullong n,uint base,char *chars);
	int printn(llong n);
	int prints(const char *str,ssize_t len);

	size_t indent;
	bool lineStart;
	static char hexCharsBig[];
	static char hexCharsSmall[];
};
