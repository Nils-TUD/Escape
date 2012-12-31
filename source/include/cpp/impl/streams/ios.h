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

#include <impl/streams/ios_base.h>
#include <streambuf>

namespace std {
	class ostream;

	/**
	 * The base-class for all input- and output-streams. Provides a state, an exception-mask,
	 * a fill-character, a stream-buffer and a tie-stream.
	 */
	class ios: public ios_base {
	public:
		typedef char char_type;
		typedef long int_type;
		typedef unsigned long pos_type;
		typedef unsigned long off_type;
		typedef streamsize size_type;

		/**
		 * Constructs an object of class ios, assigning initial values to its member
		 * objects by calling init(sb).
		 */
		explicit ios(streambuf* sb)
			: _fill(char_type()), _rdst(iostate()), _exceptions(iostate()), _tie(NULL), _rdbuf(NULL) {
			init(sb);
		}
		/**
		 * Destructor. Does not destroy rdbuf().
		 */
		virtual ~ios() {
		}

		/**
		 * @return If fail() then a null pointer; otherwise some non-null pointer to indicate
		 * success
		 */
		operator void*() const {
			return fail() ? NULL : const_cast<ios*>(this);
		}
		/**
		 * @return fail()
		 */
		bool operator !() const {
			return fail();
		}
		/**
		 * @return the error state of the stream buffer.
		 */
		iostate rdstate() const {
			return _rdst;
		}
		/**
		 * Clears the state
		 */
		void clear(iostate state = goodbit);

		/**
		 * Calls clear(rdstate() | state ) (which may throw ios::failure (27.4.2.1.1)).
		 */
		void setstate(iostate state) {
			clear(rdstate() | state);
		}
		/**
		 * @return rdstate() == 0
		 */
		bool good() const {
			return rdstate() == 0;
		}
		/**
		 * @return true if eofbit is set in rdstate().
		 */
		bool eof() const {
			return rdstate() & eofbit;
		}
		/**
		 * @return true if failbit or badbit is set in rdstate()
		 */
		bool fail() const {
			return rdstate() & (failbit | badbit);
		}
		/**
		 * @return true if badbit is set in rdstate().
		 */
		bool bad() const {
			return rdstate() & badbit;
		}

		/**
		 * @return a mask that determines what elements set in rdstate() cause exceptions to be
		 * 	thrown.
		 */
		iostate exceptions() const {
			return _exceptions;
		}
		/**
		 * Sets exceptions() to <except>. Calls clear(rdstate()).
		 */
		void exceptions(iostate except) {
			_exceptions = except;
			clear(rdstate());
		}

		/**
		 * An output sequence that is tied to (synchronized with) the sequence controlled by the
		 * stream buffer.
		 */
		ostream* tie() const {
			return _tie;
		}
		/**
		 * Sets tie() to <tiestr>.
		 *
		 * @return the previous value of tie()
		 */
		ostream* tie(ostream* tiestr) {
			ostream* old = _tie;
			_tie = tiestr;
			return old;
		}
		/**
		 * @return a pointer to the streambuf associated with the stream.
		 */
		streambuf* rdbuf() const {
			return _rdbuf;
		}
		/**
		 * Sets rdbuf() to <sb> and calls clear().
		 *
		 * @return the previous value of rdbuf()
		 */
		streambuf* rdbuf(streambuf* sb) {
			streambuf* old = _rdbuf;
			_rdbuf = sb;
			clear();
			return old;
		}
		/**
		 * Copies the state from the given stream to this one. Leaves rdstate() and rdbuf()
		 * unchanged. Raises the event copyfmt_event.
		 *
		 * @return *this
		 */
		ios& copyfmt(const ios& rhs);
		/**
		 * @return the character used to pad (fill) an output conversion to the specified field width.
		 */
		char_type fill() const {
			return _fill;
		}
		/**
		 * Sets fill() to <fillch>
		 *
		 * @return the previous value of fill()
		 */
		char_type fill(char_type ch) {
			char_type old = _fill;
			_fill = ch;
			return old;
		}
	protected:
		/**
		 * Constructs an object of this class eaving its member objects uninitialized. The object
		 * shall be initialized by calling its init member function.
		 * If it is destroyed before it has been initialized the behavior is undefined.
		 */
		ios()
			: _fill(char_type()), _rdst(iostate()), _exceptions(iostate()), _tie(NULL), _rdbuf(NULL) {
		}
		/**
		 * Initializes the object
		 */
		void init(streambuf* sb);

	private:
		ios(const ios &); // not defined
		ios& operator =(const ios &); // not defined

	private:
		char_type _fill;
		iostate _rdst;
		iostate _exceptions;
		ostream* _tie;
		streambuf* _rdbuf;
	};
}
