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

#include <impl/streams/ios.h>

namespace std {
	ios::ios(streambuf* sb)
		: _fill(char_type()), _rdst(iostate()), _exceptions(iostate()), _tie(NULL), _rdbuf(NULL) {
		init(sb);
	}
	ios::ios()
		: _fill(char_type()), _rdst(iostate()), _exceptions(iostate()), _tie(NULL), _rdbuf(NULL) {
	}
	ios::~ios() {
		// do nothing
	}

	void ios::init(streambuf* sb) {
		rdbuf(sb);
		tie(NULL);
		_rdst = sb ? goodbit : badbit;
		exceptions(goodbit);
		flags(skipws | dec | right);
		width(0);
		precision(6);
		fill(' ');
	}

	ios::operator void*() const {
		return fail() ? NULL : const_cast<ios*>(this);
	}
	bool ios::operator !() const {
		return fail();
	}

	ios::iostate ios::rdstate() const {
		return _rdst;
	}
	void ios::clear(iostate state) {
		if(rdbuf() != 0)
			_rdst = state;
		else
			_rdst = state | badbit;
		if((_rdst & exceptions()) != 0)
			throw failure("bad state");
	}
	void ios::setstate(iostate state) {
		clear(rdstate() | state);
	}

	bool ios::good() const {
		return rdstate() == 0;
	}
	bool ios::eof() const {
		return rdstate() & eofbit;
	}
	bool ios::fail() const {
		return rdstate() & (failbit | badbit);
	}
	bool ios::bad() const {
		return rdstate() & badbit;
	}

	ios::iostate ios::exceptions() const {
		return _exceptions;
	}
	void ios::exceptions(iostate except) {
		_exceptions = except;
		clear(rdstate());
	}

	ostream* ios::tie() const {
		return _tie;
	}
	ostream* ios::tie(ostream* tiestr) {
		ostream* old = _tie;
		_tie = tiestr;
		return old;
	}

	streambuf* ios::rdbuf() const {
		return _rdbuf;
	}
	streambuf* ios::rdbuf(streambuf* sb) {
		streambuf* old = _rdbuf;
		_rdbuf = sb;
		clear();
		return old;
	}

	ios& ios::copyfmt(const ios& rhs) {
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

	ios::char_type ios::fill() const {
		return _fill;
	}
	ios::char_type ios::fill(char_type ch) {
		char_type old = _fill;
		_fill = ch;
		return old;
	}
}
