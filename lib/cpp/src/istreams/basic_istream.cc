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
	// === basic_istream::sentry ===
	template<class charT,class traits>
	basic_istream<charT,traits>::sentry::sentry(basic_istream<charT,traits>& is,bool noskipws)
		: _ok(false) {
		if(is.good()) {
			//TODO if(is.tie())
			//	is.tie()->flush();
			if(noskipws == 0 && (is.flags() & ios_base::skipws)) {
				while(true) {
					int_type c = is.peek();
					if(c == traits::eof()) {
						is.setstate(basic_ios<charT,traits>::failbit | basic_ios<charT,traits>::eofbit);
						break;
					}
					if(!isspace(c))
						break;
					is.get();
				}
			}
		}
		if(is.good())
			_ok = true;
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
	inline basic_istream<charT,traits>::basic_istream(basic_streambuf<charT,traits>* sb)
		: _lastcount(0), _sb(sb) {
		basic_ios<charT,traits>::init(sb);
	}
	template<class charT,class traits>
	inline basic_istream<charT,traits>::~basic_istream() {
		// TODO what to do?
	}

	template<class charT,class traits>
	basic_istream<charT,traits>& basic_istream<charT,traits>::operator >>(
			basic_istream<charT,traits>& (*pf)(basic_istream<charT,traits>&)) {
		pf(*this);
		return *this;
	}
	template<class charT,class traits>
	basic_istream<charT,traits>& basic_istream<charT,traits>::operator >>(
			basic_ios<charT,traits>& (*pf)(basic_ios<charT,traits>&)) {
		pf(*this);
		return *this;
	}
	template<class charT,class traits>
	basic_istream<charT,traits>& basic_istream<charT,traits>::operator >>(
			ios_base & (*pf)(ios_base &)) {
		pf(*this);
		return *this;
	}

	template<class charT,class traits>
	inline basic_istream<charT,traits>& basic_istream<charT,traits>::operator >>(bool& n) {
		long l;
		readInteger(l,false);
		n = static_cast<bool>(l);
		return *this;
	}
	template<class charT,class traits>
	inline basic_istream<charT,traits>& basic_istream<charT,traits>::operator >>(short& n) {
		long l;
		readInteger(l,true);
		n = static_cast<short>(l);
		return *this;
	}
	template<class charT,class traits>
	inline basic_istream<charT,traits>& basic_istream<charT,traits>::operator >>(unsigned short& n) {
		long l;
		readInteger(l,false);
		n = static_cast<unsigned short>(l);
		return *this;
	}
	template<class charT,class traits>
	inline basic_istream<charT,traits>& basic_istream<charT,traits>::operator >>(int& n) {
		long l;
		readInteger(l,true);
		n = static_cast<int>(l);
		return *this;
	}
	template<class charT,class traits>
	inline basic_istream<charT,traits>& basic_istream<charT,traits>::operator >>(unsigned int& n) {
		long l;
		readInteger(l,false);
		n = static_cast<unsigned int>(l);
		return *this;
	}
	template<class charT,class traits>
	inline basic_istream<charT,traits>& basic_istream<charT,traits>::operator >>(long& n) {
		readInteger(n,true);
		return *this;
	}
	template<class charT,class traits>
	inline basic_istream<charT,traits>& basic_istream<charT,traits>::operator >>(unsigned long& n) {
		readInteger(static_cast<long&>(n),false);
		return *this;
	}

	template<class charT,class traits>
	void basic_istream<charT,traits>::readInteger(long &n,bool sign) {
		sentry se(*this,false);
		if(se) {
			const char *numTable = "0123456789abcdef";
			bool neg = false;
			int base = 10;
			if(ios_base::flags() & ios_base::oct)
				base = 8;
			else if(ios_base::flags() & ios_base::dec)
				base = 10;
			else if(ios_base::flags() & ios_base::hex)
				base = 16;

			/* handle '-' */
			if(sign) {
				char_type rc = _sb->peek();
				if(traits::eq(rc,'-')) {
					neg = true;
					_sb->get();
				}
			}

			/* read until an invalid char is found or the max length is reached */
			n = 0;
			while(true) {
				char_type tc = tolower(_sb->get());
				if(tc >= '0' && tc <= numTable[base - 1]) {
					if(base > 10 && tc >= 'a')
						n = n * base + (10 + tc - 'a');
					else
						n = n * base + (tc - '0');
				}
				else {
					_sb->unget();
					break;
				}
			}
			if(neg)
				n = -n;
		}
	}

	template<class charT,class traits>
	inline typename basic_istream<charT,traits>::size_type basic_istream<charT,traits>::lastcount() const {
		return _lastcount;
	}
	template<class charT,class traits>
	typename basic_istream<charT,traits>::char_type basic_istream<charT,traits>::get() {
		sentry se(*this,true);
		char_type c = traits::eof();
		if(se) {
			c = _sb->get();
			if(traits::eq(c,traits::eof()))
				basic_ios<charT,traits>::setstate(ios_base::failbit);
			else
				_lastcount = 1;
		}
		return c;
	}

	template<class charT,class traits>
	inline basic_istream<charT,traits>& basic_istream<charT,traits>::get(char_type* s,size_type n) {
		return get(s,n,'\n');
	}
	template<class charT,class traits>
	basic_istream<charT,traits>& basic_istream<charT,traits>::get(char_type* s,size_type n,
			char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			while(n > 1) {
				char_type c = _sb->peek();
				if(traits::eq(c,traits::eof())) {
					basic_ios<charT,traits>::setstate(ios_base::eofbit);
					break;
				}
				else if(traits::eq(c,delim))
					break;
				_sb->get();
				*s++ = c;
				n--;
				_lastcount++;
			}
			*s = charT();
		}
		if(_lastcount == 0)
			basic_ios<charT,traits>::setstate(ios_base::failbit);
		return *this;
	}
	template<class charT,class traits>
	inline basic_istream<charT,traits>& basic_istream<charT,traits>::get(
			basic_streambuf<char_type,traits>& sb) {
		return get(sb,'\n');
	}
	template<class charT,class traits>
	basic_istream<charT,traits>& basic_istream<charT,traits>::get(
			basic_streambuf<char_type,traits>& sb,char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			while(true) {
				try {
					char_type c = _sb->peek();
					if(traits::eq(c,traits::eof())) {
						basic_ios<charT,traits>::setstate(ios_base::eofbit);
						break;
					}
					else if(traits::eq(c,delim))
						break;
					_sb->get();
					sb.put(c);
					_lastcount++;
				}
				catch(...) {
					break;
				}
			}
		}
		if(_lastcount == 0)
			basic_ios<charT,traits>::setstate(ios_base::failbit);
		return *this;
	}

	template<class charT,class traits>
	inline basic_istream<charT,traits>& basic_istream<charT,traits>::getline(char_type* s,
			size_type n) {
		return getline(s,n,'\n');
	}
	template<class charT,class traits>
	basic_istream<charT,traits>& basic_istream<charT,traits>::getline(char_type* s,size_type n,
			char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			while(n > 1) {
				char_type c = _sb->get();
				if(traits::eq(c,traits::eof())) {
					basic_ios<charT,traits>::setstate(ios_base::eofbit);
					break;
				}
				else if(traits::eq(c,delim))
					break;
				*s++ = c;
				n--;
				_lastcount++;
			}
			*s = charT();
		}
		if(_lastcount == 0)
			basic_ios<charT,traits>::setstate(ios_base::failbit);
		return *this;
	}

	template<class charT,class traits>
	basic_istream<charT,traits>& basic_istream<charT,traits>::ignore(size_type n,char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			while(true) {
				if(n != numeric_limits<streamsize>::max()) {
					if(n == 0)
						break;
					n--;
					_lastcount++;
				}
				char_type c = _sb->get();
				if(traits::eq(c,traits::eof())) {
					basic_ios<charT,traits>::setstate(ios_base::eofbit);
					break;
				}
				else if(traits::eq(c,delim))
					break;
			}
		}
		if(_lastcount == 0)
			basic_ios<charT,traits>::setstate(ios_base::failbit);
		return *this;
	}

	template<class charT,class traits>
	typename basic_istream<charT,traits>::char_type basic_istream<charT,traits>::peek() {
		sentry se(*this,true);
		if(se) {
			if(!basic_ios<charT,traits>::good())
				return traits::eof();
			return _sb->peek();
		}
		return traits::eof();
	}
	template<class charT,class traits>
	basic_istream<charT,traits>& basic_istream<charT,traits>::unget() {
		sentry se(*this,true);
		if(se) {
			if(!basic_ios<charT,traits>::good())
				basic_ios<charT,traits>::setstate(ios_base::failbit);
			else {
				if(!_sb->unget())
					basic_ios<charT,traits>::setstate(ios_base::badbit);
			}
		}
		return *this;
	}
}
