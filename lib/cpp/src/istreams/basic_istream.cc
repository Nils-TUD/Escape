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

#include <istreams/basic_istream.h>
#include <ctype.h>

namespace std {
	// === basic_istream::sentry ===
	template<class charT,class traits>
	basic_istream<charT,traits>::sentry::sentry(basic_istream<charT,traits>& is,bool noskipws)
		: _ok(true) {
		if(is.good()) {
			if(is.tie())
				is.tie()->flush();
			if(noskipws == 0 && (is.flags() & ios_base::skipws)) {
				while(true) {
					int_type c = is.peek();
					if(c == traits::eof()) {
						is.setstate(basic_ios::failbit | basic_ios::eofbit);
						break;
					}
					if(!isspace(c))
						break;
					is.get();
				}
			}
		}
	}
	template<class charT,class traits>
	basic_istream<charT,traits>::sentry::~sentry() {

	}
	template<class charT,class traits>
	basic_istream<charT,traits>::sentry::operator bool() const {
		return _ok;
	}

	// === basic_istream ===
	template<class charT,class traits>
	basic_istream<charT,traits>::basic_istream(basic_streambuf<charT,traits>* sb)
		: _gcount(0) {
		basic_ios::init(sb);
	}
	template<class charT,class traits>
	basic_istream<charT,traits>::~basic_istream() {
		// TODO what to do?
	}
}
