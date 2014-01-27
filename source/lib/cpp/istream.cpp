/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <esc/esccodes.h>
#include <istream>
#include <ostream>
#include <limits>
#include <ctype.h>

namespace std {
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
						if(c == EOF) {
							is.setstate(ios::failbit | ios::eofbit);
							break;
						}
						buf->get();
					}
				}
			}
			catch(...) {
				is.setstate(ios::failbit);
			}
		}
		if(is.good())
			_ok = true;
	}

	// === istream ===

	istream& istream::operator >>(bool& n) {
		if(ios_base::flags() & ios_base::boolalpha)
			n = readAlphaBool();
		else
			readInteger(n,false);
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
					if(c == EOF) {
						ios::setstate(ios_base::eofbit);
						return false;
					}
					if(c != *exp)
						return false;
					exp++;
				}
				return true;
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
		}
		return false;
	}

	istream::char_type istream::get() {
		_lastcount = 0;
		sentry se(*this,true);
		char_type c = EOF;
		if(se) {
			try {
				c = _sb->get();
				if(c == EOF)
					ios::setstate(ios_base::eofbit);
				else
					_lastcount = 1;
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

	istream& istream::get(char_type* s,size_type n,char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			try {
				while(n > 1) {
					char_type c = _sb->peek();
					if(c == EOF) {
						ios::setstate(ios_base::eofbit);
						break;
					}
					if(c == delim)
						break;
					_sb->get();
					*s++ = c;
					n--;
					_lastcount++;
				}
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
	istream& istream::get(streambuf& sb,char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			try {
				while(true) {
					char_type c = _sb->peek();
					if(c == EOF) {
						ios::setstate(ios_base::eofbit);
						break;
					}
					if(c == delim)
						break;
					sb.put(c);
					_sb->get();
					_lastcount++;
				}
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
					if(c == EOF) {
						ios::setstate(ios_base::eofbit);
						break;
					}
					_lastcount++;
					if(isspace(c))
						break;
					*s++ = c;
				}
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
					if(c == EOF) {
						ios::setstate(ios_base::eofbit);
						break;
					}
					_lastcount++;
					if(isspace(c))
						break;
					sb.put(c);
				}
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

	istream& istream::getline(char_type* s,size_type n,char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			try {
				while(n > 1) {
					char_type c = _sb->get();
					if(c == EOF) {
						ios::setstate(ios_base::eofbit);
						break;
					}
					_lastcount++;
					if(c == delim)
						break;
					*s++ = c;
					n--;
				}
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

	istream& istream::getline(string& s,char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			try {
				streamsize n = ios_base::width();
				while(n == 0 || n-- > 1) {
					char_type c = _sb->get();
					if(c == EOF) {
						ios::setstate(ios_base::eofbit);
						break;
					}
					_lastcount++;
					if(c == delim)
						break;
					s += c;
				}
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

	istream& istream::getline(streambuf& sb,char_type delim) {
		_lastcount = 0;
		sentry se(*this,true);
		if(se) {
			try {
				streamsize n = ios_base::width();
				while(n == 0 || n-- > 1) {
					char_type c = _sb->get();
					if(c == EOF) {
						ios::setstate(ios_base::eofbit);
						break;
					}
					_lastcount++;
					if(c == delim)
						break;
					sb.put(c);
				}
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
					}
					char_type c = _sb->get();
					_lastcount++;
					if(c == EOF) {
						ios::setstate(ios_base::eofbit);
						break;
					}
					if(c == delim)
						break;
				}
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

	istream::esc_type istream::getesc(esc_type& n1,esc_type& n2,esc_type& n3) {
		esc_type cmd = ESCC_INVALID;
		sentry se(*this,true);
		if(se) {
			try {
				int i;
				char ec,escape[MAX_ESCC_LENGTH] = {0};
				const char *escPtr = (const char*)escape;
				for(i = 0; i < MAX_ESCC_LENGTH - 1 && (ec = _sb->get()) != ']'; i++)
					escape[i] = ec;
				if(ec == EOF)
					ios::setstate(ios_base::eofbit);
				else if(i < MAX_ESCC_LENGTH - 1 && ec == ']')
					escape[i] = ec;
				if(ec != EOF) {
					esc_type ln1,ln2,ln3;
					cmd = escc_get(&escPtr,&ln1,&ln2,&ln3);
					n1 = ln1;
					n2 = ln2;
					n3 = ln3;
				}
			}
			catch(...) {
				ios::setstate(ios_base::failbit | ios_base::badbit);
			}
		}
		return cmd;
	}

	istream::char_type istream::peek() {
		char_type c = EOF;
		sentry se(*this,true);
		if(se) {
			try {
				c = _sb->peek();
				if(c == EOF)
					ios::setstate(ios_base::eofbit);
			}
			catch(...) {
				ios::setstate(ios_base::failbit | ios_base::badbit);
			}
		}
		return c;
	}
	istream& istream::unget() {
		sentry se(*this,true);
		if(se) {
			try {
				_sb->unget();
			}
			catch(...) {
				ios::setstate(ios_base::failbit | ios_base::badbit);
			}
		}
		return *this;
	}
}
