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
#include <iosfwd>

namespace std {
	template<class charT,class traits = char_traits<charT> >
	class basic_istream: virtual public basic_ios<charT,traits> {
	public:
		/**
		 * Constructs an object of class basic_istream, assigning initial values to the base class#
		 * by calling basic_ios::init(sb)
		 * Postcondition: gcount() == 0
		 */
		explicit basic_istream(basic_streambuf<charT,traits>* sb);
		/**
		 * Effects: Destroys an object of class basic_istream.
		 * Remarks: Does not perform any operations of rdbuf().
		 */
		virtual ~basic_istream();

		class sentry {
		public:
			/**
			 * If is.good() is true, prepares for formatted or unformatted input. First, if
			 * is.tie() is not a null pointer, the function calls is.tie()->flush() to
			 * synchronize the output sequence with any associated external C stream. Except
			 * that this call can be suppressed if the put area of is.tie() is empty. Further
			 * an implementation is allowed to defer the call to flush until a call of
			 * is->rdbuf()->underflow occurs. If no such call occurs before the sentry object
			 * is destroyed, the call to flush may be eliminated entirely. If noskipws is zero and
			 * is.flags() & ios_base::skipws is nonzero, the function extracts and discards
			 * each character as long as the next available input character c is a whitespace
			 * character. If is.rdbuf()->sbumpc() or is.rdbuf()->sgetc() returns
			 * traits::eof(), the function calls setstate(failbit | eofbit) (which may throw
			 * ios_base::failure).
			 *
			 * const ctype < charT >& ctype = use_facet < ctype < charT > >(is . getloc ());
			 * if ( ctype . is ( ctype . space ,c )!=0)
			 * 	// c is a whitespace character.
			 *
			 * If, after any preparation is completed, is .good() is true, ok_ != false otherwise,
			 * ok_ == false. During preparation, the constructor may call setstate(failbit)
			 * (which may throw ios_base::failure)
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
		 * @return pf(*this)
		 */
		basic_istream& operator >>(basic_istream& (*pf)(basic_istream&));
		/**
		 * Calls pf(*this)
		 * @return *this
		 */
		basic_istream& operator >>(basic_ios<charT,traits>& (*pf)(basic_ios<charT,traits>&));
		/**
		 * Calls pf(*this)
		 * @return *this
		 */
		basic_istream& operator >>(ios_base& (*pf)(ios_base &));

		basic_istream<charT,traits>& operator >>(bool& n);
		basic_istream<charT,traits>& operator >>(short& n);
		basic_istream<charT,traits>& operator >>(unsigned short& n);
		basic_istream<charT,traits>& operator >>(int& n);
		basic_istream<charT,traits>& operator >>(unsigned int& n);
		basic_istream<charT,traits>& operator >>(long& n);
		basic_istream<charT,traits>& operator >>(unsigned long& n);
		basic_istream<charT,traits>& operator >>(float& f);
		basic_istream<charT,traits>& operator >>(double& f);
		basic_istream<charT,traits>& operator >>(long double& f);
		basic_istream<charT,traits>& operator >>(void*& p);

		/**
		 * Behaves as a formatted input function. If sb is null, calls setstate(failbit), which may
		 * throw ios_base::failure. After a sentry object is constructed, extracts characters
		 * from *this and inserts them in the output sequence controlled by sb. Characters are
		 * extracted and inserted until any of the following occurs:
		 * - end-of-file occurs on the input sequence;
		 * - inserting in the output sequence fails (in which case the character to be inserted is
		 * 	not extracted);
		 * - an exception occurs (in which case the exception is caught).
		 *
		 * If the function inserts no characters, it calls setstate(failbit), which may throw
		 * ios_base::failure. If it inserted no characters because it caught an exception thrown
		 * while extracting characters from *this and failbit is on in exceptions(), then the
		 * caught exception is rethrown.
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& operator >>(basic_streambuf<char_type,traits>* sb);

		/**
		 * @return the number of characters extracted by the last unformatted input member
		 * 	function called for the object.
		 */
		streamsize gcount() const;
		/**
		 * Behaves as an unformatted input function. After constructing a sentry object, extracts
		 * a character c, if one is available. Otherwise, the function calls setstate(failbit),
		 * which may throw ios_base::failure.
		 *
		 * @return c if available, otherwise traits::eof().
		 */
		int_type get();
		/**
		 * Behaves as an unformatted input function. After constructing a sentry object, extracts
		 * a character, if one is available, and assigns it to c. Otherwise, the function calls
		 * setstate(failbit) (which may throw ios_base::failure).
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& get(char_type& c);
		/**
		 * @return get(s ,n ,widen('\n'))
		 */
		basic_istream<charT,traits>& get(char_type* s,streamsize n);
		/**
		 * Behaves as an unformatted input function. After constructing a sentry object, extracts
		 * characters and stores them into successive locations of an array whose first element is
		 * designated by s. Characters are extracted and stored until any of the following occurs:
		 * - n - 1 characters are stored;
		 * - end-of-file occurs on the input sequence (in which case the function calls
		 * 	setstate(eofbit));
		 * - traits::eq(c,delim) for the next available input character c (in which case c is not
		 * 	extracted).
		 *
		 * If the function stores no characters, it calls setstate(failbit) (which may throw
		 * ios_base::failure). In any case, it then stores a null character into the next
		 * successive location of the array.
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& get(char_type* s,streamsize n,char_type delim);
		/**
		 * @return get(sb , widen('\n'))
		 */
		basic_istream<charT,traits>& get(basic_streambuf<char_type,traits>& sb);
		/**
		 * Behaves as an unformatted input function. After constructing a sentry object, extracts
		 * characters and inserts them in the output sequence controlled by sb . Characters are
		 * extracted and inserted until any of the following occurs:
		 * - end-of-file occurs on the input sequence;
		 * - inserting in the output sequence fails (in which case the character to be inserted is
		 * 	not extracted);
		 * - traits::eq(c,delim) for the next available input character c (in which case c is not
		 * 	extracted);
		 * - an exception occurs (in which case, the exception is caught but not rethrown).
		 *
		 * If the function inserts no characters, it calls setstate(failbit), which may throw
		 * ios_base::failure
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& get(basic_streambuf<char_type,traits>& sb,char_type delim);
		/**
		 * @return getline(s ,n ,widen('\n'))
		 */
		basic_istream<charT,traits>& getline(char_type* s,streamsize n);
		/**
		 * Behaves as an unformatted input function. After constructing a sentry object, extracts
		 * characters and stores them into successive locations of an array whose first element is
		 * designated by s. Characters are extracted and stored until one of the following occurs:
		 * 1. end-of-file occurs on the input sequence (in which case the function calls
		 * 	setstate(eofbit));
		 * 2. traits::eq(c,delim) for the next available input character c (in which case the input
		 * 	character is extracted but not stored);
		 * 3. n - 1 characters are stored (in which case the function calls setstate(failbit)).
		 *
		 * These conditions are tested in the order shown.
		 * If the function extracts no characters, it calls setstate(failbit) (which may throw
		 * ios_base::failure)
		 * In any case, it then stores a null character (using charT()) into the next successive
		 * location of the array.
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& getline(char_type* s,streamsize n,char_type delim);
		/**
		 * Behaves as an unformatted input function. After constructing a sentry object, extracts
		 * characters and discards them. Characters are extracted until any of the following occurs:
		 * - if n != numeric_limits<streamsize>::max(), n characters are extracted
		 * - end-of-file occurs on the input sequence (in which case the function calls
		 * 	setstate(eofbit), which may throw ios_base::failure);
		 * - traits::eq_int_type(traits::to_int_type(c ), delim ) for the next available input
		 * 	character c (in which case c is extracted).
		 *
		 * The last condition will never occur if traits::eq_int_type(delim , traits::eof()).
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& ignore(streamsize n = 1,int_type delim = traits::eof());
		/**
		 * Behaves as an unformatted input function. After constructing a sentry object, reads
		 * but does not extract the current input character.
		 *
		 * @return traits::eof() if good() is false. Otherwise, returns rdbuf()->sgetc().
		 */
		int_type peek();
		/**
		 * Behaves as an unformatted input function. After constructing a sentry object, if !good()
		 * calls setstate(failbit) which may throw an exception, and return. Otherwise extracts
		 * characters and stores them into successive locations of an array whose first element is
		 * designated by s. Characters are extracted and stored until either of the following occurs:
		 * - n characters are stored;
		 * - end-of-file occurs on the input sequence (in which case the function calls
		 * 	setstate(failbit|eofbit), which may throw ios_base::failure).
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& read(char_type* s,streamsize n);
		/**
		 * Behaves as an unformatted input function. After constructing a sentry object, if !good()
		 * calls setstate(failbit) which may throw an exception, and return. Otherwise extracts
		 * characters and stores them into successive locations of an array whose first element
		 * is designated by s. If rdbuf()->in_avail() == -1, calls setstate(eofbit) (which may
		 * throw ios_base::failure), and extracts no characters;
		 * - If rdbuf()->in_avail() == 0, extracts no characters
		 * - If rdbuf()->in_avail() > 0, extracts min(rdbuf()->in_avail(),n )).
		 *
		 * @return the number of characters extracted.
		 */
		streamsize readsome(char_type* s,streamsize n);
		/**
		 * Behaves as an unformatted input function. After constructing a sentry object, if !good()
		 * calls setstate(failbit) which may throw an exception, and return. If rdbuf() is not
		 * null, calls rdbuf->sputbackc(). If rdbuf() is null, or if sputbackc() returns
		 * traits::eof(), calls setstate(badbit) (which may throw ios_base::failure).
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& putback(char_type c);
		/**
		 * Behaves as an unformatted input function. After constructing a sentry object, if !good()
		 * calls setstate(failbit) which may throw an exception, and return. If rdbuf() is not
		 * null, calls rdbuf()->sungetc(). If rdbuf() is null, or if sungetc() returns
		 * traits::eof(), calls setstate(badbit) (which may throw ios_base::failure).
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

		/**
		 * Behaves as an unformatted input function, except that it does not count the number
		 * of characters extracted and does not affect the value returned by subsequent calls to
		 * gcount().
		 * @return After constructing a sentry object, if fail() != false, returns pos_type(-1) to
		 * indicate failure. Otherwise, returns rdbuf()->pubseekoff(0, cur , in ).
		 */
		pos_type tellg();
		/**
		 * Behaves as an unformatted input function, except that it does not count the number of
		 * characters extracted and does not affect the value returned by subsequent calls to
		 * gcount(). After constructing a sentry object, if fail() != true, executes
		 * rdbuf()->pubseekpos(pos ,ios_base::in). In case of failure, the function calls
		 * setstate(failbit) (which may throw ios_base failure).
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& seekg(pos_type);
		/**
		 * Behaves as an unformatted input function, except that it does not count the number of
		 * characters extracted and does not affect the value returned by subsequent calls to
		 * gcount(). After constructing a sentry object, if fail() != true, executes
		 * rdbuf()->pubseekoff(off , dir , ios_base::in).
		 *
		 * @return *this
		 */
		basic_istream<charT,traits>& seekg(off_type,ios_base::seekdir);

	private:
		streamsize _gcount;
	};

	template<class charT,class traits>
	basic_istream<charT,traits>& operator >>(basic_istream<charT,traits>&,charT &);
	template<class traits>
	basic_istream<char,traits>& operator >>(basic_istream<char,traits>&,unsigned char &);
	template<class traits>
	basic_istream<char,traits>& operator >>(basic_istream<char,traits>&,signed char &);
	template<class charT,class traits>
	basic_istream<charT,traits>& operator >>(basic_istream<charT,traits>&,charT *);
	template<class traits>
	basic_istream<char,traits>& operator >>(basic_istream<char,traits>&,unsigned char *);
	template<class traits>
	basic_istream<char,traits>& operator >>(basic_istream<char,traits>&,signed char *);
}

#include "../../../lib/cpp/src/istreams/basic_istream.cc"

#endif /* BASIC_ISTREAM_H_ */
