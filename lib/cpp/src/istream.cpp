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

#include <istream>
#include <ostream>
#include <limits>
#include <ctype.h>

namespace esc {
	// === istream::sentry ===
	istream::sentry::sentry(istream& is,bool noskipws)
		: _ok(false) {
		if(is.good()) {
			try {
				if(is.tie())
					is.tie()->flush();
				if(noskipws == 0 && (is.flags() & ios_base::skipws)) {
					streambuf* buf = is.rdbuf();
					while(true) {
						int_type c = buf->peek();
						if(!isspace(c))
							break;
						buf->get();
					}
				}
			}
			catch(eof_reached&) {
				is.setstate(ios::failbit | ios::eofbit);
			}
			catch(...) {
				is.setstate(ios::failbit);
			}
		}
		if(is.good())
			_ok = true;
	}
	istream::sentry::~sentry() {
	}
	istream::sentry::operator bool() const {
		return _ok;
	}

	// === istream ===
	istream::istream(streambuf* sb)
		: _lastcount(0), _sb(sb) {
		ios::init(sb);
	}
	istream::~istream() {
		// TODO what to do?
	}

	istream& istream::operator >>(istream& (*pf)(istream&)) {
		pf(*this);
		return *this;
	}
	istream& istream::operator >>(ios& (*pf)(ios&)) {
		pf(*this);
		return *this;
	}
	istream& istream::operator >>(ios_base & (*pf)(ios_base &)) {
		pf(*this);
		return *this;
	}

	istream& istream::operator >>(bool& n) {
		if(ios_base::flags() & ios_base::boolalpha)
			n = readAlphaBool();
		else {
			long l;
			readInteger(l,false);
			n = static_cast<bool>(l);
		}
		return *this;
	}
	istream& istream::operator >>(short& n) {
		long l;
		readInteger(l,true);
		n = static_cast<short>(l);
		return *this;
	}
	istream& istream::operator >>(unsigned short& n) {
		long l;
		readInteger(l,false);
		n = static_cast<unsigned short>(l);
		return *this;
	}
	istream& istream::operator >>(int& n) {
		long l;
		readInteger(l,true);
		n = static_cast<int>(l);
		return *this;
	}
	istream& istream::operator >>(unsigned int& n) {
		long l;
		readInteger(l,false);
		n = static_cast<unsigned int>(l);
		return *this;
	}
	istream& istream::operator >>(long& n) {
		readInteger(n,true);
		return *this;
	}
	istream& istream::operator >>(unsigned long& n) {
		readInteger((long&)(n),false);
		return *this;
	}

	bool istream::readAlphaBool() {
		char_type c = _sb->peek();
		if(c == 't')
			return readString("true");
		readString("false");
		return false;
	}

	bool istream::readString(const char *exp) {
		sentry se(*this,false);
		if(se) {
			try {
				while(*exp) {
					char_type c = _sb->get();
					if(c != *exp)
						return false;
					exp++;
				}
				return true;
			}
			catch(eof_reached&) {
				ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
		}
		return false;
	}

	void istream::readInteger(long &n,bool sign) {
		sentry se(*this,false);
		if(se) {
			bool neg = false;
			try {
				static const char *numTable = "0123456789abcdef";
				int base = ios_base::get_base();

				// handle '-'
				if(sign) {
					char_type rc = _sb->peek();
					if(rc == '-') {
						neg = true;
						_sb->get();
					}
				}

				// read until an invalid char is found or the max length is reached
				n = 0;
				int remain = ios_base::width();
				if(remain == 0)
					remain = -1;
				while(remain < 0 || remain-- > 0) {
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
			}
			catch(eof_reached&) {
				ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
			if(neg)
				n = -n;
			ios_base::width(0);
		}
	}

	istream::size_type istream::lastcount() const {
		return _lastcount;
	}
	istream::char_type istream::get() {
		_lastcount = 0;
		sentry se(*this,true);
		char_type c = EOF;
		if(se) {
			try {
				c = _sb->get();
				_lastcount = 1;
			}
			catch(eof_reached&) {
				ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
		}
		if(_lastcount == 0)
			ios::setstate(ios_base::failbit);
		ios_base::width(0);
		return c;
	}

	istream& istream::get(char_type* s,size_type n) {
		return get(s,n,'\n');
	}
	istream& istream::get(char_type* s,size_type n,char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			try {
				while(n > 1) {
					char_type c = _sb->peek();
					if(c == delim)
						break;
					_sb->get();
					*s++ = c;
					n--;
					_lastcount++;
				}
			}
			catch(eof_reached&) {
				ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
			*s = '\0';
		}
		if(_lastcount == 0)
			ios::setstate(ios_base::failbit);
		ios_base::width(0);
		return *this;
	}
	istream& istream::get(streambuf& sb) {
		return get(sb,'\n');
	}
	istream& istream::get(streambuf& sb,char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			try {
				while(true) {
					char_type c = _sb->peek();
					if(c == delim)
						break;
					_sb->get();
					sb.put(c);
					_lastcount++;
				}
			}
			catch(eof_reached&) {
				ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
		}
		if(_lastcount == 0)
			ios::setstate(ios_base::failbit);
		ios_base::width(0);
		return *this;
	}

	istream& istream::getword(char_type* s) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			streamsize n = ios_base::width();
			try {
				while(n == 0 || n-- > 1) {
					char_type c = _sb->get();
					if(isspace(c))
						break;
					*s++ = c;
					_lastcount++;
				}
			}
			catch(eof_reached&) {
				ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
			*s = '\0';
		}
		if(_lastcount == 0)
			ios::setstate(ios_base::failbit);
		ios_base::width(0);
		return *this;
	}

	istream& istream::getword(streambuf& sb) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			streamsize n = ios_base::width();
			try {
				while(n == 0 || n-- > 1) {
					char_type c = _sb->get();
					if(isspace(c))
						break;
					sb.put(c);
					_lastcount++;
				}
			}
			catch(eof_reached&) {
				ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
		}
		if(_lastcount == 0)
			ios::setstate(ios_base::failbit);
		ios_base::width(0);
		return *this;
	}

	istream& istream::getline(char_type* s,size_type n) {
		return getline(s,n,'\n');
	}
	istream& istream::getline(char_type* s,size_type n,
			char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			try {
				while(n > 1) {
					char_type c = _sb->get();
					if(c == delim)
						break;
					*s++ = c;
					n--;
					_lastcount++;
				}
			}
			catch(eof_reached&) {
				ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
			*s = '\0';
		}
		if(_lastcount == 0)
			ios::setstate(ios_base::failbit);
		ios_base::width(0);
		return *this;
	}

	istream& istream::getline(streambuf& sb,char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			try {
				streamsize n = ios_base::width();
				while(n == 0 || n-- > 1) {
					char_type c = _sb->get();
					if(c == delim)
						break;
					sb.put(c);
					_lastcount++;
				}
			}
			catch(eof_reached&) {
				ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
		}
		if(_lastcount == 0)
			ios::setstate(ios_base::failbit);
		ios_base::width(0);
		return *this;
	}

	istream& istream::ignore(size_type n,char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			try {
				while(true) {
					if(n != numeric_limits<streamsize>::max()) {
						if(n == 0)
							break;
						n--;
						_lastcount++;
					}
					char_type c = _sb->get();
					if(c == delim)
						break;
				}
			}
			catch(eof_reached&) {
				ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
		}
		if(_lastcount == 0)
			ios::setstate(ios_base::failbit);
		ios_base::width(0);
		return *this;
	}

	istream::char_type istream::peek() {
		sentry se(*this,true);
		if(se) {
			try {
				return _sb->peek();
			}
			catch(eof_reached&) {
				ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::failbit | ios_base::badbit);
			}
		}
		return EOF;
	}
	istream& istream::unget() {
		sentry se(*this,true);
		if(se) {
			try {
				_sb->unget();
			}
			catch(eof_reached&) {
				ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::failbit | ios_base::badbit);
			}
		}
		return *this;
	}

	istream& operator >>(istream& in,char& c) {
		ws(in);
		c = in.get();
		return in;
	}
	istream& operator >>(istream& in,char* s) {
		ws(in);
		return in.getword(s);
	}

	istream& ws(istream& is) {
		istream::sentry se(is,false);
		return is;
	}
}
