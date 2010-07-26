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

#ifndef BASIC_IOS_H_
#define BASIC_IOS_H_

#include <istreams/ios_base.h>
#include <istreams/ios_types.h>
#include <istreams/basic_streambuf.h>

namespace std {
	template<class charT,class traits>
	class basic_ios: public ios_base {
	public:
		typedef charT char_type;
		typedef typename traits::int_type int_type;
		typedef typename traits::pos_type pos_type;
		typedef typename traits::off_type off_type;
		typedef streamsize size_type;
		typedef traits traits_type;

		/**
		 * @return If fail() then a null pointer; otherwise some non-null pointer to indicate
		 * success
		 */
		operator void*() const;	// void* unspecified
		/**
		 * @return fail()
		 */
		bool operator !() const;
		/**
		 * @return the error state of the stream buffer.
		 */
		iostate rdstate() const;
		/**
		 * Clears the state
		 */
		void clear(iostate state = goodbit);

		/**
		 * Calls clear(rdstate() | state ) (which may throw basic_ios::failure (27.4.2.1.1)).
		 */
		void setstate(iostate state);
		/**
		 * @return rdstate() == 0
		 */
		bool good() const;
		/**
		 * @return true if eofbit is set in rdstate().
		 */
		bool eof() const;
		/**
		 * @return true if failbit or badbit is set in rdstate()
		 */
		bool fail() const;
		/**
		 * @return true if badbit is set in rdstate().
		 */
		bool bad() const;

		/**
		 * @return a mask that determines what elements set in rdstate() cause exceptions to be
		 * 	thrown.
		 */
		iostate exceptions() const;
		/**
		 * Sets exceptions() to <except>. Calls clear(rdstate()).
		 */
		void exceptions(iostate except);

		/**
		 * Constructs an object of class basic_ios, assigning initial values to its member
		 * objects by calling init(sb).
		 */
		explicit basic_ios(basic_streambuf<charT,traits>* sb);
		/**
		 * Destructor. Does not destroy rdbuf().
		 */
		virtual ~basic_ios();

		/**
		 * An output sequence that is tied to (synchronized with) the sequence controlled by the
		 * stream buffer.
		 */
		//basic_ostream<charT,traits>* tie() const;
		/**
		 * Sets tie() to <tiestr>.
		 *
		 * @return the previous value of tie()
		 */
		//basic_ostream<charT,traits>* tie(basic_ostream<charT,traits>* tiestr);
		/**
		 * @return a pointer to the streambuf associated with the stream.
		 */
		basic_streambuf<charT,traits>* rdbuf() const;
		/**
		 * Sets rdbuf() to <sb> and calls clear().
		 *
		 * @return the previous value of rdbuf()
		 */
		basic_streambuf<charT,traits>* rdbuf(basic_streambuf<charT,traits>* sb);
		/**
		 * Copies the state from the given stream to this one. Leaves rdstate() and rdbuf()
		 * unchanged. Raises the event copyfmt_event.
		 *
		 * @return *this
		 */
		basic_ios& copyfmt(const basic_ios& rhs);
		/**
		 * @return the character used to pad (fill) an output conversion to the specified field width.
		 */
		char_type fill() const;
		/**
		 * Sets fill() to <fillch>
		 *
		 * @return the previous value of fill()
		 */
		char_type fill(char_type ch);
	protected:
		/**
		 * Constructs an object of this class eaving its member objects uninitialized. The object
		 * shall be initialized by calling its init member function.
		 * If it is destroyed before it has been initialized the behavior is undefined.
		 */
		basic_ios();
		/**
		 * Initializes the object
		 */
		void init(basic_streambuf<charT,traits>* sb);

	private:
		basic_ios(const basic_ios &); // not defined
		basic_ios& operator =(const basic_ios &); // not defined

	private:
		char_type _fill;
		iostate _rdst;
		iostate _exceptions;
		//basic_ostream<charT,traits>* _tie;
		basic_streambuf<charT,traits>* _rdbuf;
	};
}

#include "../../../lib/cpp/src/istreams/basic_ios.cc"

#endif /* BASIC_IOS_H_ */
