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

#include <sys/common.h>
#include <esc/stream/iosbase.h>
#include <esc/stream/ostringstream.h>
#include <ctype.h>
#include <string>
#include <assert.h>

namespace esc {

/**
 * The input-stream is used to read formatted input from various sources. Subclasses have
 * to implement the method to actually read a character. This class provides the higher-level
 * stuff around it.
 * Note that fmt() can be used here too (see OStream), although many flags are just ignored because
 * they don't make any sense for input.
 */
class IStream : public virtual IOSBase {
	template<typename T>
	friend class IOSBase::FormatImplPtr;
	template<typename T>
	friend class IOSBase::FormatImplUint;
	template<typename T>
	friend class IOSBase::FormatImplInt;
	template<typename T>
	friend class IOSBase::FormatImplFloat;
	template<typename T>
	friend class IOSBase::FormatImplStr;
	template<typename T>
	friend class IOSBase::FormatImplStrObj;

public:
	explicit IStream() : IOSBase() {
	}
	virtual ~IStream() {
	}

	/**
	 * No cloning
	 */
	IStream(const IStream&) = delete;
	IStream &operator=(const IStream&) = delete;

	/**
	 * But moving is supported
	 */
	IStream(IStream &&i) : IOSBase(std::move(i)) {
	}
	IStream &operator=(IStream &&i) {
		if(&i != this)
			IOSBase::operator=(std::move(i));
		return *this;
	}

	/**
	 * Reads a value out of the stream and stores it in <val>.
	 *
	 * @param val the value
	 * @return *this
	 */
	IStream &operator>>(char &val) {
		val = read();
		return *this;
	}
	IStream &operator>>(uchar &val) {
		return read_unsigned(val);
	}
	IStream &operator>>(ushort &val) {
		return read_unsigned(val);
	}
	IStream &operator>>(short &val) {
		return read_signed(val);
	}
	IStream &operator>>(uint &val) {
		return read_unsigned(val);
	}
	IStream &operator>>(int &val) {
		return read_signed(val);
	}
	IStream &operator>>(ulong &val) {
		return read_unsigned(val);
	}
	IStream &operator>>(long &val) {
		return read_signed(val);
	}
	IStream &operator>>(ullong &val) {
		return readu<ullong>(val);
	}
	IStream &operator>>(llong &val) {
		return readn<llong>(val);
	}
	IStream &operator>>(float &val) {
		return read_float(val);
	}
	IStream &operator>>(double &val) {
		return read_float(val);
	}

	/**
	 * Reads a string until the next newline is found.
	 *
	 * @param str will be set to the read string
	 * @return *this
	 */
	IStream &operator>>(std::string &str) {
		getword(str);
		return *this;
	}

	/**
	 * Reads a word into <str>, i.e. it consumes all characters until a space character is found.
	 *
	 * @param str the string to write to
	 * @param flags additional flags (IStream::Flags::*)
	 * @return the number of written characters (excluding '\0')
	 */
	size_t getword(std::string &str,int flags = 0) {
		str.clear();
		char c;
		if(~flags & Flags::KEEPWS)
			ignore_ws();
		while(1) {
			c = read();
			if(bad() || isspace(c))
				break;
			str.append(1,c);
		}
		return str.length();
	}

	/**
	 * Reads a string into <str> until <delim> is found.
	 *
	 * @param str the string to write to
	 * @param delim the delimited ('\n' by default)
	 * @return the number of written characters (excluding '\0')
	 */
	size_t getline(std::string &str,char delim = '\n') {
		str.clear();
		char c;
		while(1) {
			c = read();
			if(bad() || c == delim)
				break;
			str.append(1,c);
		}
		return str.length();
	}

	/**
	 * Reads a string into <buffer> until <delim> is found or <max> characters have been stored
	 * to <buffer>, including '\0'.
	 *
	 * @param buffer the destination
	 * @param max the maximum number of bytes to store to <buffer>
	 * @param delim the delimited ('\n' by default)
	 * @return the number of written characters (excluding '\0')
	 */
	size_t getline(char *buffer,size_t max,char delim = '\n') {
		assert(max >= 1);
		char *begin = buffer;
		char *end = buffer + max - 1;
		do {
			*buffer = read();
			if(bad() || *buffer == delim)
				break;
			buffer++;
		}
		while(buffer < end);
		*buffer = '\0';
		return buffer - begin;
	}

	/**
	 * Reads an escape-code from the stream. Assumes that the last read char was '\033'.
	 *
	 * @param n1 will be set to the first argument (ESCC_ARG_UNUSED if unused)
	 * @param n2 will be set to the second argument (ESCC_ARG_UNUSED if unused)
	 * @param n3 will be set to the third argument (ESCC_ARG_UNUSED if unused)
	 * @return the scanned escape-code (ESCC_*)
	 */
	int getesc(int &n1,int &n2,int &n3);

	/**
	 * Ignores all characters until <delim> is found, but at most <max>.
	 *
	 * @param max the maximum characters to ignore (1 by default)
	 * @param delim the delimited ('\n' by default)
	 */
	IStream &ignore(size_t max = 1,char delim = '\n') {
		while(max-- > 0) {
			char c = read();
			if(bad() || c == delim)
				break;
		}
		return *this;
	}

	/**
	 * Ignores all whitespace characters.
	 *
	 * @return *this
	 */
	IStream &ignore_ws() {
		char c = read();
		while(good() && isspace(c))
			c = read();
		if(good())
			putback(c);
		return *this;
	}

	/**
	 * Alias for read().
	 */
	char get() {
		return read();
	}

	/**
	 * Reads something from the string with formatting applied. This operator should be used in
	 * combination with fmt():
	 * sin >> fmt(val,"x");
	 */
	template<typename T>
	IStream &operator>>(const Format<T>& fmt) {
		FormatParams p(fmt.fmt());
		FormatImpl<T>().read(*this,p,fmt.value());
		return *this;
	}

	/**
	 * Reads one character from the stream.
	 *
	 * @return the character
	 */
	virtual char read() = 0;

	/**
	 * Reads <count> bytes into <dst>.
	 *
	 * @param dst the destination to read into
	 * @param count the number of bytes to read
	 * @return the number of read bytes
	 */
	virtual size_t read(void *dst,size_t count) = 0;

	/**
	 * Puts <c> back into the stream. This is guaranteed to work at least once after one character
	 * has been read, if the same character is put back. In other cases, it might fail, depending
	 * on the backend.
	 *
	 * @param c the character to put back
	 * @return true on success
	 */
	virtual bool putback(char c) = 0;

private:
	template<typename T>
	IStream &read_unsigned(T &u,uint base = 0) {
		ulong tmp;
		readu<ulong>(tmp,base);
		u = tmp;
		return *this;
	}

	template<typename T>
	IStream &read_signed(T &n) {
		long tmp;
		readn<long>(tmp);
		n = tmp;
		return *this;
	}

	template<typename T>
	IStream &read_float(T &f) {
		double d;
		readf(d);
		f = d;
		return *this;
	}

	template<typename T>
	IStream &readu(T &u,uint base = 0) {
		ignore_ws();
		u = 0;
		char c = read();
		if(base == 0) {
			base = 10;
			if(c == '0') {
				base = 8;
				if((c = read()) == 'x' || c == 'X') {
					base = 16;
					c = read();
				}
				else if(!isdigit(c)) {
					putback(c);
					c = '0';
				}
			}
		}

		// ensure that we consume at least one character
		if(isdigit(c) || (base == 16 && isxdigit(c))) {
			do {
				if(c >= 'a' && c <= 'f')
					u = u * base + c + 10 - 'a';
				else if(c >= 'A' && c <= 'F')
					u = u * base + c + 10 - 'A';
				else
					u = u * base + c - '0';
				c = read();
			}
			while(good() && (isdigit(c) || (base == 16 && isxdigit(c))));
			putback(c);
		}
		return *this;
	}

	template<typename T>
	IStream &readn(T &n) {
		bool neg = false;
		ignore_ws();

		char c = read();
		if(c == '-' || c == '+') {
			neg = c == '-';
			c = read();
		}

		n = 0;
		if(isdigit(c)) {
			do {
				n = n * 10 + c - '0';
				c = read();
			}
			while(good() && isdigit(c));
			putback(c);
		}

		// switch sign?
		if(neg)
			n = -n;
		return *this;
	}

	IStream &readptr(uintptr_t &ptr);
	IStream &readf(double &f);
};

template<typename T>
void IOSBase::FormatImplPtr<T>::read(IStream &is,const FormatParams &,T &value) {
	uintptr_t ptr;
	is.readptr(ptr);
	value = ptr;
}

template<typename T>
void IOSBase::FormatImplUint<T>::read(IStream &is,const FormatParams &p,T &value) {
	// let the user print an integer as a pointer if a wants to. this saves a cast to void*
	if(p.flags() & IOSBase::POINTER) {
		uintptr_t ptr;
		is.readptr(ptr);
		value = ptr;
	}
	// although we rely on the type in most cases, we let the user select between signed
	// and unsigned by specifying certain flags that are only used at one place.
	// this free's the user from having to be really careful whether a value is signed or
	// unsigned, which is especially a problem when using typedefs.
	else if(p.flags() & (IOSBase::FORCESIGN | IOSBase::SPACESIGN))
		is.read_signed(value);
	else
		is.read_unsigned(value,p.base());
}

template<typename T>
void IOSBase::FormatImplInt<T>::read(IStream &is,const FormatParams &p,T &value) {
	// like above; the base is only used in unsigned print, so do that if the user specified
	// a base (10 is default)
	if(p.base() != 10)
		is.read_unsigned(value,p.base());
	else
		is.read_signed(value);
}

template<typename T>
void IOSBase::FormatImplFloat<T>::read(IStream &is,const FormatParams &,T &value) {
	is.read_float(value);
}

template<typename T>
void IOSBase::FormatImplStrObj<T>::read(IStream &is,const FormatParams &p,T &value) {
	is.getword(value,p.flags());
}

/**
 * Creates a Format-object that can be written into IStream to read something from the stream into
 * the given value with specified formatting parameters. This function exists to allow
 * template parameter type inference.
 *
 * Example usage:
 * uint val;
 * sin >> fmt(val,"x");
 *
 * @param value the value to format
 * @param fmt the format parameters
 * @param pad the number of padding characters (default 0)
 * @param precision the precision (default -1 = none)
 * @return the Format object
 */
template<typename T>
static inline IOSBase::Format<T> fmt(T &value,const char *fmt) {
	return IOSBase::Format<T>(fmt,value,0,-1);
}

}
