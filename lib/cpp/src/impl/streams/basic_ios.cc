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

namespace std {
	template<class charT,class traits>
	inline basic_ios<charT,traits>::basic_ios(basic_streambuf<charT,traits>* sb) {
		init(sb);
	}
	template<class charT,class traits>
	inline basic_ios<charT,traits>::basic_ios() {
		// do nothing
	}
	template<class charT,class traits>
	inline basic_ios<charT,traits>::~basic_ios() {
		// do nothing
	}

	template<class charT,class traits>
	void basic_ios<charT,traits>::init(basic_streambuf<charT,traits>* sb) {
		rdbuf(sb);
		tie(NULL);
		_rdst = sb ? goodbit : badbit;
		exceptions(goodbit);
		flags(skipws | dec | right);
		width(0);
		precision(6);
		fill(' ');
	}

	template<class charT,class traits>
	inline basic_ios<charT,traits>::operator void*() const {
		return fail() ? NULL : const_cast<basic_ios<charT,traits>*>(this);
	}
	template<class charT,class traits>
	inline bool basic_ios<charT,traits>::operator !() const {
		return fail();
	}

	template<class charT,class traits>
	inline typename basic_ios<charT,traits>::iostate basic_ios<charT,traits>::rdstate() const {
		return _rdst;
	}
	template<class charT,class traits>
	void basic_ios<charT,traits>::clear(iostate state) {
		if(rdbuf() != 0)
			_rdst = state;
		else
			_rdst = state | badbit;
		if((_rdst & exceptions()) != 0)
			throw failure("bad state");
	}
	template<class charT,class traits>
	inline void basic_ios<charT,traits>::setstate(iostate state) {
		clear(rdstate() | state);
	}

	template<class charT,class traits>
	inline bool basic_ios<charT,traits>::good() const {
		return rdstate() == 0;
	}
	template<class charT,class traits>
	inline bool basic_ios<charT,traits>::eof() const {
		return rdstate() & eofbit;
	}
	template<class charT,class traits>
	inline bool basic_ios<charT,traits>::fail() const {
		return rdstate() & (failbit | badbit);
	}
	template<class charT,class traits>
	inline bool basic_ios<charT,traits>::bad() const {
		return rdstate() & badbit;
	}

	template<class charT,class traits>
	inline typename basic_ios<charT,traits>::iostate basic_ios<charT,traits>::exceptions() const {
		return _exceptions;
	}
	template<class charT,class traits>
	inline void basic_ios<charT,traits>::exceptions(iostate except) {
		_exceptions = except;
		clear(rdstate());
	}

	template<class charT,class traits>
	inline basic_ostream<charT,traits>* basic_ios<charT,traits>::tie() const {
		return _tie;
	}
	template<class charT,class traits>
	inline basic_ostream<charT,traits>* basic_ios<charT,traits>::tie(
			basic_ostream<charT,traits>* tiestr) {
		basic_ostream<charT,traits>* old = _tie;
		_tie = tiestr;
		return old;
	}

	template<class charT,class traits>
	inline basic_streambuf<charT,traits>* basic_ios<charT,traits>::rdbuf() const {
		return _rdbuf;
	}
	template<class charT,class traits>
	inline basic_streambuf<charT,traits>* basic_ios<charT,traits>::rdbuf(
			basic_streambuf<charT,traits>* sb) {
		basic_streambuf<charT,traits>* old = _rdbuf;
		_rdbuf = sb;
		clear();
		return old;
	}

	template<class charT,class traits>
	basic_ios<charT,traits>& basic_ios<charT,traits>::copyfmt(const basic_ios<charT,traits>& rhs) {
		if(this != &rhs) {
			raise_event(erase_event);
			flags(rhs.flags());
			precision(rhs.precision());
			width(rhs.width());
			fill(rhs.fill());
			tie(rhs.tie());
			raise_event(copyfmt_event);
			exceptions(rhs.exceptions());
		}
		return *this;
	}

	template<class charT,class traits>
	inline typename basic_ios<charT,traits>::char_type basic_ios<charT,traits>::fill() const {
		return _fill;
	}
	template<class charT,class traits>
	inline typename basic_ios<charT,traits>::char_type basic_ios<charT,traits>::fill(char_type ch) {
		char_type old = _fill;
		_fill = ch;
		return old;
	}
}
