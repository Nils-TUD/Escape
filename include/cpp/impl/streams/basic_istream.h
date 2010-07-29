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

#ifndef BASIC_ISTREAM_H_
#define BASIC_ISTREAM_H_

#include <stddef.h>
#include <impl/streams/basic_ios.h>
#include <impl/streams/basic_stringbuf.h>
#include <impl/streams/ios_base.h>
#include <limits>
#include <ctype.h>

namespace std {
	/**
	 * The basic input-stream that provides formated- and unformated-input-methods.
	 */
	template<class charT,class traits = char_traits<charT> >
	class basic_istream: virtual public basic_ios<charT,traits> {
	public:
		typedef typename basic_ios<charT,traits>::char_type char_type;
		typedef typename basic_ios<charT,traits>::int_type int_type;
		typedef typename basic_ios<charT,traits>::size_type size_type;

		/**
		 * Constructs an object of class basic_istream with given stream-buffer
		 */
		explicit basic_istream(basic_streambuf<charT,traits>* sb);
		/**
		 * Destructor. Does not perform any operations of rdbuf().
		 */
		virtual ~basic_istream();

		/**
		 * For pre- and post-operations
		 */
		class sentry {
		public:
			/**
			 * If is.good() is true, prepares for formatted or unformatted input. If desired the
			 * function extracts and discards whitespace-characters
			 */
			explicit sentry(basic_istream<charT,traits>& is,bool noskipws = false);
			/**
			 * Does nothing
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
		};

		/**
		 * Calls pf(*this) and returns *this
		 */
		basic_istream<charT,traits>& operator >>(
				basic_istream<charT,traits>& (*pf)(basic_istream<charT,traits>&));
		basic_istream<charT,traits>& operator >>(
				basic_ios<charT,traits>& (*pf)(basic_ios<charT,traits>&));
		basic_istream<charT,traits>& operator >>(ios_base & (*pf)(ios_base &));

		/**
		 * Reads an integer or floating-point-number from the stream into the given integer-
		 * or floating-point-variable.
		 * Sets ios_base::badbit if something went wrong and ios_base::eofbit if EOF has been
		 * reached. This may throw an exception (depending on basic_ios::exceptions())
		 *
		 * @param n the integer to write into
		 * @param f the floating-point-number to write into
		 * @param p the pointer to write into
		 * @return *this
		 */
		basic_istream<charT,traits>& operator >>(bool& n);
		basic_istream<charT,traits>& operator >>(short& n);
		basic_istream<charT,traits>& operator >>(unsigned short& n);
		basic_istream<charT,traits>& operator >>(int& n);
		basic_istream<charT,traits>& operator >>(unsigned int& n);
		basic_istream<charT,traits>& operator >>(long& n);
		basic_istream<charT,traits>& operator >>(unsigned long& n);
		/* TODO
		 * basic_istream<charT,traits>& operator >>(float& f);
		basic_istream<charT,traits>& operator >>(double& f);
		basic_istream<charT,traits>& operator >>(long double& f);
		basic_istream<charT,traits>& operator >>(void*& p);*/

		/**
		 * Extracts characters and stores them into the given streambuffer. The method stops
		 * if EOF occurrs, inserting fails or an exception occurs.
		 *
		 * @param sb the streambuffer
		 * @return *this
		 */
		// TODO basic_istream<charT,traits>& operator >>(basic_streambuf<charT,traits>* sb);

		/**
		 * @return the number of characters extracted by the last unformatted input member
		 * 	function called for the object.
		 */
		size_type lastcount() const;
		/**
		 * Extracts a character c, if one is available. Otherwise, the function calls
		 * setstate(failbit), which may throw ios_base::failure.
		 *
		 * @return c if available, otherwise traits::eof().
		 */
		char_type get();
		/**
		 * @return get(s ,n ,widen('\n'))
		 */
		basic_istream<charT,traits>& get(char_type* s,size_type n);
		/**
		 * Extracts characters and stores them into successive locations of an array whose first
		 * element is designated by <s>. The method stops if <n>-1 characters are stored, EOF occurrs
		 * or <delim> is found.
		 * If the function stores no characters, it calls setstate(failbit) (which may throw
		 * ios_base::failure). In any case, it then stores a null character into the next
		 * successive location of the array.
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& get(char_type* s,size_type n,char_type delim);
		/**
		 * @return get(sb , widen('\n'))
		 */
		basic_istream<charT,traits>& get(basic_streambuf<char_type,traits>& sb);
		/**
		 * Extracts characters and inserts them in the output sequence controlled by <sb>. The method
		 * stops if inserting fails, EOF is reached, an exception occurs or <delim> is found.
		 * If the function inserts no characters, it calls setstate(failbit), which may throw
		 * ios_base::failure. Note that <delim> is NOT extracted and NOT stored!
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& get(basic_streambuf<char_type,traits>& sb,char_type delim);
		/**
		 * @return getline(s ,n ,widen('\n'))
		 */
		basic_istream<charT,traits>& getline(char_type* s,size_type n);
		/**
		 * Extracts characters and stores them into successive locations of an array whose first
		 * element is designated by <s>. The method stops if <n>-1 characters are stored, EOF is
		 * reached, or <delim> is found.
		 * If the function extracts no characters, it calls setstate(failbit) (which may throw
		 * ios_base::failure). In any case, it then stores a null character into the next
		 * successive location of the array.
		 * Note that <delim> is extracted but NOT stored!
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& getline(char_type* s,size_type n,char_type delim);
		/**
		 * Extracts characters and discards them until <n> characters have been discarded (n =
		 * numeric_limits<streamsize>::max() means unlimited) or EOF is reached or <delim>
		 * is found. Note that <delim> is extracted!
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& ignore(size_type n = 1,char_type delim = traits::eof());
		/**
		 * Reads but does not extract the current input character.
		 *
		 * @return traits::eof() if good() is false. Otherwise the character
		 */
		char_type peek();
		/**
		 * Moves the stream one position back
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& unget();
		/**
		 * Behaves as an unformatted input function, except that it does not count the number
		 * of characters extracted and does not affect the value returned by subsequent calls to
		 * gcount(). After constructing a sentry object, if rdbuf() is a null pointer,
		 * returns -1 . Otherwise, calls rdbuf()->pubsync() and, if that function returns
		 * -1 calls setstate(badbit) (which may throw ios_base::failure, and returns -1.
		 * Otherwise, returns zero.
		 */
		int sync();

	private:
		void readInteger(long &n,bool sign);
		bool readAlphaBool();
		bool readString(const charT *exp);

	private:
		size_type _lastcount;
		basic_streambuf<charT,traits>* _sb;
	};

	/**
	 * Reads and extracts the current character and stores it into <c>
	 *
	 * @param in the stream
	 * @param c the reference to the character
	 * @return in
	 */
	template<class charT,class traits>
	basic_istream<charT,traits>& operator >>(basic_istream<charT,traits>& in,charT& c);
	template<class traits>
	basic_istream<char,traits>& operator >>(basic_istream<char,traits>& in,unsigned char& c);
	template<class traits>
	basic_istream<char,traits>& operator >>(basic_istream<char,traits>& in,signed char& c);

	/**
	 * Reads and extracts characters until either EOF is reached, an error occurrs or a
	 * whitespace-character is found. If width() is not 0, it stops when width() is reached.
	 * In any case, <s> is terminated by a null-character
	 *
	 * @param in the stream
	 * @param s the string to store the characters in
	 * @return *this
	 */
	template<class charT,class traits>
	basic_istream<charT,traits>& operator >>(basic_istream<charT,traits>& in,charT* s);
	template<class traits>
	basic_istream<char,traits>& operator >>(basic_istream<char,traits>& in,unsigned char* s);
	template<class traits>
	basic_istream<char,traits>& operator >>(basic_istream<char,traits>& in,signed char* s);
	template<class charT,class traits>
	basic_istream<charT,traits>& operator >>(basic_istream<charT,traits>& in,basic_string<charT>& s);

	/**
	 * Discards whitespace
	 *
	 * @param is the stream
	 * @return is
	 */
	template<class charT,class traits>
	basic_istream<charT,traits>& ws(basic_istream<charT,traits>& is);
}

#include "../../../../lib/cpp/src/impl/streams/basic_istream.cc"

#endif /* BASIC_ISTREAM_H_ */
