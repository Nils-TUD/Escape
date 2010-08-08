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

#include <ostream>
#include <esc/width.h>
#include <string.h>

#define FFL_SHORT			1
#define FFL_LONG			2

namespace std {
	ostream::sentry::sentry(ostream& os)
			: _ok(false), _os(os) {
		if(os.good()) {
			if(os.tie())
				os.tie()->flush();
		}
		if(os.good())
			_ok = true;
	}
	ostream::sentry::~sentry() {
		if((_os.flags() & ios_base::unitbuf) && !std::uncaught_exception())
			_os.flush();
	}
	ostream::sentry::operator bool() const {
		return _ok;
	}

	ostream::ostream(streambuf* sb)
			: _sb(sb) {
		ios::init(sb);
	}
	ostream::~ostream() {
	}

	ostream& ostream::format(const char *fmt,...) {
		va_list ap;
		va_start(ap,fmt);
		ostream& res = format(fmt,ap);
		va_end(ap);
		return res;
	}

	ostream& ostream::format(const char *fmt,va_list ap) {
		sentry se(*this);
		if(se) {
			bool readFlags;
			ios_base::fmtflags fflags;
			s16 prec;
			char c,*str,b;
			s32 n;
			u32 u;
			double d;
			u8 intsize,pad;

			ios_base::fmtflags oldflags = ios_base::flags();
			streamsize oldprec = ios_base::precision();
			streamsize oldwidth = ios_base::width();
			char_type oldfill = ios::fill();

			try {
				while(1) {
					/* wait for a '%' */
					while((c = *fmt++) != '%') {
						/* finished? */
						if(c == '\0')
							goto done;
						_sb->put(c);
					}

					/* read flags */
					fflags = ios_base::dec | ios_base::right | ios_base::skipws;
					pad = 0;
					readFlags = true;
					while(readFlags) {
						switch(*fmt) {
							case '-':
								fflags &= ~ios_base::right;
								fflags |= ios_base::left;
								fmt++;
								break;
							case '+':
								fflags |= ios_base::showpos;
								fmt++;
								break;
							case '#':
								fflags |= ios_base::showbase;
								fmt++;
								break;
							case '0':
								ios::fill('0');
								fmt++;
								break;
							case '*':
								pad = (u8)va_arg(ap, u32);
								fmt++;
								break;
							default:
								readFlags = false;
								break;
						}
					}

					/* read pad-width */
					if(pad == 0) {
						while(*fmt >= '0' && *fmt <= '9') {
							pad = pad * 10 + (*fmt - '0');
							fmt++;
						}
					}

					/* read precision */
					prec = 6;
					if(*fmt == '.') {
						fmt++;
						prec = 0;
						while(*fmt >= '0' && *fmt <= '9') {
							prec = prec * 10 + (*fmt - '0');
							fmt++;
						}
					}

					// set formatting settings
					ios_base::precision(prec);
					ios_base::width(pad);
					ios_base::flags(fflags);

					/* read length */
					intsize = 0;
					switch(*fmt) {
						case 'l':
							intsize |= FFL_LONG;
							fmt++;
							break;
						case 'h':
							intsize |= FFL_SHORT;
							fmt++;
							break;
					}

					/* format */
					switch((c = *fmt++)) {
						/* signed integer */
						case 'd':
						case 'i':
							n = va_arg(ap, s32);
							if(fflags & FFL_SHORT)
								n &= 0xFFFF;
							writeSigned(n);
							break;

						/* pointer */
						case 'p':
							u = va_arg(ap, u32);
							// TODO
							writeUnsigned(u);
							break;

						/* floating points */
						case 'f':
							/* 'float' is promoted to 'double' when passed through '...' */
							d = va_arg(ap, double);
							writeDouble(d);
							break;

						/* unsigned integer */
						case 'b':
						case 'u':
						case 'o':
						case 'x':
						case 'X':
							if(c == 'o')
								ios_base::setf(ios_base::oct);
							else if(c == 'x' || c == 'X') {
								if(c == 'X')
									ios_base::setf(ios_base::uppercase);
								ios_base::setf(ios_base::hex);
							}
							u = va_arg(ap, u32);
							if(intsize & FFL_SHORT)
								u &= 0xFFFF;
							writeUnsigned(u);
							break;

						/* string */
						case 's':
							str = va_arg(ap, char*);
							write(str,strlen(str));
							break;

						/* character */
						case 'c':
							b = (char)va_arg(ap, u32);
							_sb->put(b);
							break;

						default:
							_sb->put(c);
							break;
					}
				}
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}

			done:
			ios_base::flags(oldflags);
			ios_base::width(oldwidth);
			ios_base::precision(oldprec);
			ios::fill(oldfill);
		}
		return *this;
	}

	ostream& ostream::operator <<(ostream& (*pf)(ostream&)) {
		pf(*this);
		return *this;
	}
	ostream& ostream::operator <<(ios& (*pf)(ios&)) {
		pf(*this);
		return *this;
	}
	ostream& ostream::operator <<(ios_base & (*pf)(ios_base &)) {
		pf(*this);
		return *this;
	}

	ostream& ostream::operator <<(bool n) {
		if(ios_base::flags() & ios_base::boolalpha)
			write(n ? "true" : "false",n ? SSTRLEN("true") : SSTRLEN("false"));
		else
			writeUnsigned(n);
		return *this;
	}
	ostream& ostream::operator <<(short n) {
		writeSigned(n);
		return *this;
	}
	ostream& ostream::operator <<(unsigned short n) {
		writeUnsigned(n);
		return *this;
	}
	ostream& ostream::operator <<(int n) {
		writeSigned(n);
		return *this;
	}
	ostream& ostream::operator <<(unsigned int n) {
		writeUnsigned(n);
		return *this;
	}
	ostream& ostream::operator <<(long n) {
		writeSigned(n);
		return *this;
	}
	ostream& ostream::operator <<(unsigned long n) {
		writeUnsigned(n);
		return *this;
	}
	ostream& ostream::operator <<(float f) {
		writeDouble(f);
		return *this;
	}
	ostream& ostream::operator <<(double f) {
		writeDouble(f);
		return *this;
	}
	ostream& ostream::operator <<(long double f) {
		writeDouble(f);
		return *this;
	}
	ostream& ostream::operator <<(const void* p) {
		writeUnsigned((unsigned long)(p));
		return *this;
	}

	ostream& ostream::operator <<(streambuf* sb) {
		sentry se(*this);
		if(se) {
			streamsize n = sb->available();
			streamsize m = n;
			streamsize pwidth = ios_base::width();
			try {
				if((ios_base::flags() & ios_base::right) && pwidth > n)
					writePad(pwidth - n);
				try {
					while(m-- > 0)
						_sb->put(sb->get());
				}
				catch(eof_reached&) {
					// simply stop
				}
				if((ios_base::flags() & ios_base::left) && pwidth > n)
					writePad(pwidth - n);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
			ios_base::width(0);
		}
		return *this;
	}

	ostream& ostream::put(char_type c) {
		return write(&c,1);
	}
	ostream& ostream::write(const char_type* s,streamsize n) {
		sentry se(*this);
		if(se) {
			streamsize pwidth = ios_base::width();
			try {
				streamsize m = n;
				if((ios_base::flags() & ios_base::right) && pwidth > n)
					writePad(pwidth - n);
				while(m-- > 0)
					_sb->put(*s++);
				if((ios_base::flags() & ios_base::left) && pwidth > n)
					writePad(pwidth - n);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
			ios_base::width(0);
		}
		return *this;
	}

	ostream& ostream::flush() {
		try {
			_sb->flush();
		}
		catch(...) {
			ios::setstate(ios_base::badbit);
		}
		return *this;
	}

	void ostream::writeSigned(signed long n) {
		// write as unsigned if oct or hex is desired
		if(!(ios_base::flags() & ios_base::dec)) {
			writeUnsigned(static_cast<unsigned long>(n));
			return;
		}
		sentry se(*this);
		if(se) {
			try {
				streamsize nwidth = 0;
				streamsize pwidth = ios::width();
				// determine width
				if((ios_base::flags() & (ios_base::left | ios_base::right)) && pwidth > 0) {
					nwidth = getnwidth(n);
					if(ios_base::flags() & ios_base::showpos)
						nwidth++;
				}
				// pad left
				if((ios_base::flags() & ios_base::right) && pwidth > nwidth)
					writePad(pwidth - nwidth);
				// print '+' or ' ' instead of '-'
				if(ios_base::flags() & ios_base::showpos)
					_sb->put('+');
				// print number
				writeSChars(n);
				// pad right
				if((ios_base::flags() & ios_base::left) && pwidth > nwidth)
					writePad(pwidth - nwidth);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
			ios_base::width(0);
		}
	}

	void ostream::writeUnsigned(unsigned long u) {
		static const char *numTableSmall = "0123456789abcdef";
		static const char *numTableBig = "0123456789ABCDEF";
		sentry se(*this);
		if(se) {
			try {
				int base = ios_base::get_base();
				// determine width
				streamsize nwidth = 0;
				streamsize pwidth = ios::width();
				if((ios_base::flags() & (ios_base::left | ios_base::right)) && pwidth > 0) {
					nwidth = getuwidth(u,base);
					if(u > 0 && (ios_base::flags() & ios_base::showbase)) {
						switch(base) {
							case 16:
								nwidth += 2;
								break;
							case 8:
								nwidth += 1;
								break;
						}
					}
				}
				// pad left - spaces
				if((ios_base::flags() & ios_base::right) && pwidth > nwidth)
					writePad(pwidth - nwidth);
				if(u > 0 && (ios_base::flags() & ios_base::showbase)) {
					switch(base) {
						case 16:
							_sb->put('0');
							_sb->put((ios_base::flags() & ios_base::uppercase) ? 'X' : 'x');
							break;
						case 8:
							_sb->put('0');
							break;
					}
				}
				// print number
				if(ios_base::flags() & ios_base::uppercase)
					writeUChars(u,base,numTableBig);
				else
					writeUChars(u,base,numTableSmall);
				// pad right
				if((ios_base::flags() & ios_base::left) && pwidth > nwidth)
					writePad(pwidth - nwidth);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
			ios_base::width(0);
		}
	}

	void ostream::writeDouble(long double d) {
		sentry se(*this);
		if(se) {
			try {
				long long pre = (long long)d;
				streamsize nwidth = 0;
				streamsize pwidth = ios::width();
				if(!(ios_base::flags() & (ios_base::right | ios_base::left)) && pwidth > 0) {
					nwidth = getlwidth(pre) + ios::precision() + 1;
					if(d >= 0 && (ios_base::flags() & ios_base::showpos))
						nwidth++;
				}
				// pad left
				if((ios_base::flags() & ios_base::right) && pwidth > nwidth)
					writePad(pwidth - nwidth);
				if(d >= 0 && (ios_base::flags() & ios_base::showpos))
					_sb->put('+');
				// print number
				writeDoubleChars(d);
				// pad right
				if((ios_base::flags() & ios_base::left) && pwidth > nwidth)
					writePad(pwidth - nwidth);
			}
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
			ios_base::width(0);
		}
	}

	void ostream::writeSChars(signed long n) {
		if(n < 0) {
			_sb->put('-');
			n = -n;
		}
		if(n >= 10)
			writeSChars(n / 10);
		_sb->put('0' + (n % 10));
	}

	void ostream::writeUChars(unsigned long u,unsigned int base,const char *hexchars) {
		if(u >= base)
			writeUChars(u / base,base,hexchars);
		_sb->put(hexchars[(u % base)]);
	}

	void ostream::writeDoubleChars(long double d) {
		long long val = 0;

		// Note: this is probably not a really good way of converting IEEE754-floating point numbers
		// to string. But its simple and should be enough for my purposes :)

		val = (long long)d;
		writeSChars(val);
		d -= val;
		if(d < 0)
			d = -d;
		_sb->put('.');
		streamsize prec = ios::precision();
		while(prec-- > 0) {
			d *= 10;
			val = (long long)d;
			_sb->put((val % 10) + '0');
			d -= val;
		}
	}

	void ostream::writePad(streamsize count) {
		char_type c = ios::fill();
		while(count-- > 0)
			_sb->put(c);
	}

	ostream& operator <<(ostream& os,char c) {
		os.put(c);
		return os;
	}
	ostream& operator <<(ostream& os,const char* s) {
		os.write(s,strlen(s));
		return os;
	}

	ostream& endl(ostream& os) {
		os.put('\n');
		os.flush();
		return os;
	}
	ostream& ends(ostream& os) {
		os.put('\0');
		return os;
	}
	ostream& flush(ostream& os) {
		os.flush();
		return os;
	}
}
