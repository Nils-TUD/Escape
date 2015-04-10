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
#include <stdarg.h>
#include <string>

namespace esc {

/**
 * The output-stream is used to write formatted output to various destinations. Subclasses have
 * to implement the method to actually write a character. This class provides the higher-level
 * stuff around it.
 *
 * Note that there is no printf-like formatting, but all values are put via shift operators into
 * the stream. Because printf() is not type-safe, which means you can easily make mistakes and
 * won't notice it. I know that the compiler offers a function attribute to mark printf-like
 * functions so that he can warn us about wrong usages. The problem is that it gets really
 * cumbersome to not produce warnings (so that you actually notice a new warning) when building
 * for multiple architectures. One way to get it right is embedding the length (L, l, ...) into
 * the string via preprocessor defines that depend on the architecture (think of uint64_t). Another
 * way is to add more length-specifier for uintX_t's and similar types. But it gets worse: typedefs.
 * To pass values with typedef-types to printf() in a correct way you would have to look at the
 * "implementation" of the typedef, i.e. the type it is mapped to. This makes it really hard to
 * change typedefs afterwards. So, to avoid that you would have to introduce a length modifier for
 * each typedef in your code. Not only that you'll run out of ASCII chars, this is simply not
 * acceptable. In summary, there is no reasonable way to specify the correct type for printf().
 *
 * Therefore we go a different way here. We only use shift operators and provide a quite
 * concise and simple way to add formatting parameters (in contrast to the really verbose concept
 * of the standard C++ iostreams). This way we have type-safety (you can't accidently output a
 * string as an integer or the other way around) and it's still convenient enough to pass formatting
 * parameters. This is done via template specialization. We provide a freestanding function named
 * fmt() to make use of template parameter type inference which receives all necessary information,
 * wraps them into an object and writes this object into the stream. The method for that will create
 * the corresponding template class, depending on the type to print, which in turn will finally
 * print the value.
 * The whole formatting stuff is done by OStream::FormatParams. It receives a string with formatting
 * arguments (same syntax as printf). It does not support padding and precision and recognizes only
 * the base instead of the type (i.e. only x, o, and so on and not s, c, ...). The padding and
 * precision are always passed in a separate parameter to allow "dynamic" values and only have one
 * place to specify them.
 * There is still one problem: the difference between signed and unsigned values tends to be ignored
 * by programmers. That is, I guess most people will assume that fmt(0x1234, "x") prints it with
 * a hexadecimal base. Strictly speaking, this is wrong, because 0x1234 is an int, not unsigned int,
 * and only unsigned integers are printed in a base different than 10. Thus, one would have to use
 * fmt(0x1234U, "x") to achieve it. This is even worse for typedefs or when not passing a literal,
 * because you might not even know whether its signed or unsigned. To prevent that problem I've
 * decided to interpret some formatting parameters as hints. Since the base will only be considered
 * for unsigned values, fmt(0x1234, "x") will print 0x1234 as an unsigned integer in base 16.
 * Similarly, when passing "+" or " " it will be printed as signed, even when its unsigned. And
 * finally, "p" forces a print as a pointer (xxxx:xxxx) even when its some unsigned type or char*.
 *
 * The basic syntax is: [flags][type]
 * Where [flags] is any combination of:
 * - '-': add padding on the right side instead of on the left
 * - '+': always print the sign (+ or -); forces a signed print
 * - ' ': print a space in front of positive values; forces a signed print
 * - '#': print the base
 * - '0': use zeros for padding instead of spaces
 * - 'w': keep whitespace (only for istream)
 *
 * [type] can be:
 * - 'p':           a pointer; forces a pointer print
 * - 'b':           an unsigned integer, printed in base 2; forces an unsigned print
 * - 'o':           an unsigned integer, printed in base 8; forces an unsigned print
 * - 'x':           an unsigned integer, printed in base 16; forces an unsigned print
 * - 'X':           an unsigned integer, printed in base 16 with capital letters; forces an
 *                  unsigned print
 */
class OStream : public virtual IOSBase {
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
	explicit OStream() : IOSBase() {
	}
	virtual ~OStream() {
	}

	/**
	 * No copying
	 */
	OStream(const OStream&) = delete;
	OStream &operator=(const OStream&) = delete;

	/**
	 * But moving is supported
	 */
	OStream(OStream &&o) : IOSBase(std::move(o)) {
	}
	OStream &operator=(OStream &&o) {
		if(&o != this)
			IOSBase::operator=(std::move(o));
		return *this;
	}

	/**
	 * Writes a value into the stream with formatting applied. This operator should be used in
	 * combination with fmt():
	 * sout << fmt(0x123,"x") << "\n";
	 */
	template<typename T>
	OStream &operator<<(const Format<T>& fmt) {
		FormatParams p(fmt.fmt());
		p.padding(fmt.padding());
		p.precision(fmt.precision());
		FormatImpl<T>().write(*this,p,fmt.value());
		return *this;
	}

	/**
	 * Writes the given character/integer into the stream, without formatting
	 */
	OStream &operator<<(char c) {
		write(c);
		return *this;
	}
	OStream &operator<<(uchar u) {
		return operator<<(static_cast<ulong>(u));
	}
	OStream &operator<<(short n) {
		return operator<<(static_cast<long>(n));
	}
	OStream &operator<<(ushort u) {
		return operator<<(static_cast<ulong>(u));
	}
	OStream &operator<<(int n) {
		return operator<<(static_cast<long>(n));
	}
	OStream &operator<<(uint u) {
		return operator<<(static_cast<ulong>(u));
	}
	OStream &operator<<(long n) {
		printn(n);
		return *this;
	}
	OStream &operator<<(llong n) {
		printn(n);
		return *this;
	}
	OStream &operator<<(ulong u) {
		printu(u,10,_hexchars_small);
		return *this;
	}
	OStream &operator<<(ullong u) {
		printu(u,10,_hexchars_small);
		return *this;
	}
	OStream &operator<<(float f) {
		printdbl(f,3,0,0);
		return *this;
	}
	OStream &operator<<(double d) {
		printdbl(d,3,0,0);
		return *this;
	}

	/**
	 * Writes the given string into the stream
	 */
	OStream &operator<<(const char *str) {
		puts(str);
		return *this;
	}
	OStream &operator<<(const std::string &str) {
		puts(str.c_str());
		return *this;
	}
	/**
	 * Writes the given pointer into the stream (xxxx:xxxx)
	 */
	OStream &operator<<(const void *p) {
		printptr(reinterpret_cast<uintptr_t>(p),0);
		return *this;
	}

	/**
	 * Calls the given function
	 *
	 * @param pf the function
	 */
	OStream& operator <<(OStream& (*pf)(OStream&)) {
		pf(*this);
		return *this;
	}

	/**
	 * Writes the given character into the stream.
	 *
	 * @param c the character
	 */
	virtual void write(char c) = 0;

	/**
	 * Writes <count> bytes from <src> into the stream.

	 * @param src the data to write
	 * @param count the number of bytes to write
	 * @return the number of written bytes
	 */
	virtual size_t write(const void *src,size_t count) = 0;

	/**
	 * Flushes the stream
	 */
	virtual void flush() {
	}

private:
	int printsignedprefix(long n,uint flags);
	int putspad(const char *s,uint pad,uint prec,uint flags);
	int printnpad(llong n,uint pad,uint flags);
	int printupad(ullong u,uint base,uint pad,uint flags);
	int printpad(int count,uint flags);
	int printu(ullong n,uint base,char *chars);
	int printn(llong n);
	int printdbl(double d,uint precision,uint pad,int flags);
	int printptr(uintptr_t u,uint flags);
	int puts(const char *str,ulong prec = -1);

	static char _hexchars_big[];
	static char _hexchars_small[];
};

template<typename T>
void IOSBase::FormatImplPtr<T>::write(OStream &os,const FormatParams &p,T value) {
	os.printptr(reinterpret_cast<uintptr_t>(value),p.flags());
}

template<typename T>
void IOSBase::FormatImplUint<T>::write(OStream &os,const FormatParams &p,const T &value) {
	// let the user print an integer as a pointer if a wants to. this saves a cast to void*
	if(p.flags() & IOSBase::POINTER)
		os.printptr(value,p.flags());
	// although we rely on the type in most cases, we let the user select between signed
	// and unsigned by specifying certain flags that are only used at one place.
	// this free's the user from having to be really careful whether a value is signed or
	// unsigned, which is especially a problem when using typedefs.
	else if(p.flags() & (IOSBase::FORCESIGN | IOSBase::SPACESIGN))
		os.printnpad(value,p.padding(),p.flags());
	else
		os.printupad(value,p.base(),p.padding(),p.flags());
}

template<typename T>
void IOSBase::FormatImplInt<T>::write(OStream &os,const FormatParams &p,const T &value) {
	// like above; the base is only used in unsigned print, so do that if the user specified
	// a base (10 is default)
	if(p.base() != 10)
		os.printupad(value,p.base(),p.padding(),p.flags());
	else
		os.printnpad(value,p.padding(),p.flags());
}

template<typename T>
void IOSBase::FormatImplFloat<T>::write(OStream &os,const FormatParams &p,const T &value) {
	os.printdbl(value,p.precision(),p.padding(),p.flags());
}

template<typename T>
void IOSBase::FormatImplStr<T>::write(OStream &os,const FormatParams &p,const T &value) {
	if(p.flags() & IOSBase::POINTER)
		os.printptr(reinterpret_cast<uintptr_t>(value),p.flags());
	else
		os.putspad(value,p.padding(),p.precision(),p.flags());
}

template<typename T>
void IOSBase::FormatImplStrObj<T>::write(OStream &os,const FormatParams &p,const T &value) {
	os.putspad(value.c_str(),p.padding(),p.precision(),p.flags());
}

/**
 * Creates a Format-object that can be written into OStream to write the given value into the
 * stream with specified formatting parameters. This function exists to allow template parameter
 * type inference.
 *
 * Example usage:
 * sout << "Hello " << fmt(0x1234,"#0x",8) << "\n";
 *
 * @param value the value to format
 * @param fmt the format parameters
 * @param pad the number of padding characters (default 0)
 * @param precision the precision (default -1 = none)
 * @return the Format object
 */
template<typename T>
static inline IOSBase::Format<T> fmt(const T &value,const char *fmt,uint pad = 0,uint prec = -1) {
	return IOSBase::Format<T>(fmt,value,pad,prec);
}
template<typename T>
static inline IOSBase::Format<T> fmt(const T &value,uint pad = 0,uint prec = -1) {
	return IOSBase::Format<T>("",value,pad,prec);
}

static inline OStream &endl(OStream &os) {
	os << '\n';
	os.flush();
	return os;
}

}
