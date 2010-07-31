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

#ifndef BASIC_OSTREAM_H_
#define BASIC_OSTREAM_H_

#include <stddef.h>
#include <impl/streams/basic_ios.h>
#include <impl/streams/ios_base.h>
#include <impl/streams/basic_stringbuf.h>
#include <exception>
#include <esc/width.h>

namespace std {
	/**
	 * The basic output-stream that provides formated- and unformated-output-methods.
	 */
	template<class charT,class traits = char_traits<charT> >
	class basic_ostream: virtual public basic_ios<charT,traits> {
	public:
		typedef typename basic_ios<charT,traits>::char_type char_type;
		typedef typename basic_ios<charT,traits>::int_type int_type;
		typedef typename basic_ios<charT,traits>::size_type size_type;
		typedef typename basic_ios<charT,traits>::pos_type pos_type;
		typedef typename basic_ios<charT,traits>::off_type off_type;

		/**
		 * Constructs a new output-stream with given streambuffer
		 */
		explicit basic_ostream(basic_streambuf<char_type,traits>* sb);
		/**
		 * Destructor
		 */
		virtual ~basic_ostream();

		/**
		 * For pre- and post-operations
		 */
		class sentry {
		public:
			/**
			 * If os.good() is nonzero, prepares for formatted or unformatted output.
			 */
			explicit sentry(basic_ostream<charT,traits>& os);
			/**
			 * Destructor
			 */
			~sentry();
			/**
			 * @return wether everything is ok
			 */
			operator bool() const;
		private:
			sentry(const sentry &); // not defined
			sentry & operator =(const sentry &); // not defined

		private:
			bool _ok;
			basic_ostream<charT,traits>& _os;
		};

		/**
		 * Provides printf()-like formatting.
		 *
		 * @param fmt the format
		 * @param ap the argument-pointer
		 * @return *this
		 */
		basic_ostream<charT,traits>& format(const char *fmt,...);
		basic_ostream<charT,traits>& format(const char *fmt,va_list ap);

		/**
		 * Calls pf(*this) and returns *this
		 */
		basic_ostream<charT,traits>& operator <<(basic_ostream<charT,traits>& (*pf)(basic_ostream<
				charT,traits>&));
		basic_ostream<charT,traits>& operator <<(basic_ios<charT,traits>& (*pf)(basic_ios<charT,
				traits>&));
		basic_ostream<charT,traits>& operator <<(ios_base & (*pf)(ios_base &));

		/**
		 * Writes the given integer or floating-point-number into the output-stream
		 *
		 * @param n the integer
		 * @param f the floating-point-number
		 * @param p the pointer
		 */
		basic_ostream<charT,traits>& operator <<(bool n);
		basic_ostream<charT,traits>& operator <<(short n);
		basic_ostream<charT,traits>& operator <<(unsigned short n);
		basic_ostream<charT,traits>& operator <<(int n);
		basic_ostream<charT,traits>& operator <<(unsigned int n);
		basic_ostream<charT,traits>& operator <<(long n);
		basic_ostream<charT,traits>& operator <<(unsigned long n);
		basic_ostream<charT,traits>& operator <<(float f);
		basic_ostream<charT,traits>& operator <<(double f);
		basic_ostream<charT,traits>& operator <<(long double f);
		basic_ostream<charT,traits>& operator <<(const void * p);

		basic_ostream<charT,traits>& operator <<(basic_streambuf<charT,traits>* sb);

		/**
		 * Writes the given character into the output-stream
		 *
		 * @param c the character
		 * @return *this
		 */
		basic_ostream<charT,traits>& put(char_type c);
		/**
		 * Writes <n> characters from the given string into the stream
		 *
		 * @param s the string
		 * @param n the number of chars to write
		 * @return *this
		 */
		basic_ostream<charT,traits>& write(const char_type * s,streamsize n);
		/**
		 * Flushes the buffer
		 */
		basic_ostream<charT,traits>& flush();

		/* TODO
		 * pos_type tellp();
		basic_ostream<charT,traits>& seekp(pos_type);
		basic_ostream<charT,traits>& seekp(off_type,ios_base::seekdir);*/

	private:
		void writeSigned(signed long n);
		void writeUnsigned(unsigned long u);
		void writeDouble(long double d);
		void writeSChars(signed long n);
		void writeUChars(unsigned long u,int base,const char *hexchars);
		void writeDoubleChars(long double d);
		void writePad(streamsize count);

	private:
		basic_streambuf<charT,traits>* _sb;
	};

	/**
	 * Writes the given character into the given stream
	 */
	template<class charT,class traits>
	basic_ostream<charT,traits>& operator <<(basic_ostream<charT,traits>&,charT);
	template<class charT,class traits>
	basic_ostream<charT,traits>& operator <<(basic_ostream<charT,traits>&,char);
	template<class traits>
	basic_ostream<char,traits>& operator <<(basic_ostream<char,traits>&,char);
	template<class traits>
	basic_ostream<char,traits>& operator <<(basic_ostream<char,traits>&,signed char);
	template<class traits>
	basic_ostream<char,traits>& operator <<(basic_ostream<char,traits>&,unsigned char);

	/**
	 * Writes the given string into the given stream
	 */
	template<class charT,class traits>
	basic_ostream<charT,traits>& operator <<(basic_ostream<charT,traits>&,const charT *);
	template<class charT,class traits>
	basic_ostream<charT,traits>& operator <<(basic_ostream<charT,traits>&,const char *);
	template<class traits>
	basic_ostream<char,traits>& operator <<(basic_ostream<char,traits>&,const char *);
	template<class traits>
	basic_ostream<char,traits>& operator <<(basic_ostream<char,traits>&,const signed char *);
	template<class traits>
	basic_ostream<char,traits>& operator <<(basic_ostream<char,traits>&,const unsigned char *);

	/**
	 * Writes a newline into the given stream
	 */
	template<class charT,class traits>
	basic_ostream<charT,traits>& endl(basic_ostream<charT,traits>& os);
	/**
	 * Writes a null-character into the given stream
	 */
	template<class charT,class traits>
	basic_ostream<charT,traits>& ends(basic_ostream<charT,traits>& os);
	/**
	 * Flushes the given stream
	 */
	template<class charT,class traits>
	basic_ostream<charT,traits>& flush(basic_ostream<charT,traits>& os);
}

#include "../../../../lib/cpp/src/impl/streams/basic_ostream.cc"

#endif /* BASIC_OSTREAM_H_ */
