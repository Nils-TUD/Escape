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

#define FFL_SHORT			1
#define FFL_LONG			2

namespace std {
	template<class charT,class traits>
	basic_ostream<charT,traits>::sentry::sentry(basic_ostream<charT,traits>& os)
			: _ok(false), _os(os) {
		if(os.good()) {
			if(os.tie())
				os.tie()->flush();
		}
		if(os.good())
			_ok = true;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>::sentry::~sentry() {
		if((_os.flags() & ios_base::unitbuf) && !uncaught_exception())
			_os.flush();
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>::sentry::operator bool() const {
		return _ok;
	}

	template<class charT,class traits>
	basic_ostream<charT,traits>::basic_ostream(basic_streambuf<char_type,traits>* sb)
			: _sb(sb) {
		basic_ios<charT,traits>::init(sb);
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>::~basic_ostream() {
	}

	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::format(const char *fmt,...) {
		va_list ap;
		va_start(ap,fmt);
		basic_ostream<charT,traits>& res = format(fmt,ap);
		va_end(ap);
		return res;
	}

	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::format(const char *fmt,va_list ap) {
		sentry se(*this);
		if(se) {
			bool readFlags;
			ios_base::fmtflags flags;
			s16 precision;
			char c,*str,b;
			s32 n;
			u32 u;
			double d;
			u8 intsize,pad;

			ios_base::fmtflags oldflags = ios_base::flags();
			streamsize oldprec = ios_base::precision();
			streamsize oldwidth = ios_base::width();
			char_type oldfill = basic_ios<charT,traits>::fill();

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
					flags = ios_base::dec | ios_base::right | ios_base::skipws;
					pad = 0;
					readFlags = true;
					while(readFlags) {
						switch(*fmt) {
							case '-':
								flags &= ~ios_base::right;
								flags |= ios_base::left;
								fmt++;
								break;
							case '+':
								flags |= ios_base::showpos;
								fmt++;
								break;
							case '#':
								flags |= ios_base::showbase;
								fmt++;
								break;
							case '0':
								basic_ios<charT,traits>::fill('0');
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
					precision = 6;
					if(*fmt == '.') {
						fmt++;
						precision = 0;
						while(*fmt >= '0' && *fmt <= '9') {
							precision = precision * 10 + (*fmt - '0');
							fmt++;
						}
					}

					// set formatting settings
					ios_base::precision(precision);
					ios_base::width(pad);
					ios_base::flags(flags);

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
							if(flags & FFL_SHORT)
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
				basic_ios<charT,traits>::setstate(ios_base::badbit);
			}

			done:
			ios_base::flags(oldflags);
			ios_base::width(oldwidth);
			ios_base::precision(oldprec);
			basic_ios<charT,traits>::fill(oldfill);
		}
		return *this;
	}

	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(
			basic_ostream<charT,traits>& (*pf)(basic_ostream<charT,traits>&)) {
		pf(*this);
		return *this;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(
			basic_ios<charT,traits>& (*pf)(basic_ios<charT,traits>&)) {
		pf(*this);
		return *this;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(
			ios_base & (*pf)(ios_base &)) {
		pf(*this);
		return *this;
	}

	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(bool n) {
		if(ios_base::flags() & ios_base::boolalpha)
			write(n ? "true" : "false",n ? SSTRLEN("true") : SSTRLEN("false"));
		else
			writeUnsigned(n);
		return *this;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(short n) {
		writeSigned(n);
		return *this;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(unsigned short n) {
		writeUnsigned(n);
		return *this;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(int n) {
		writeSigned(n);
		return *this;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(unsigned int n) {
		writeUnsigned(n);
		return *this;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(long n) {
		writeSigned(n);
		return *this;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(unsigned long n) {
		writeUnsigned(n);
		return *this;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(float f) {
		writeDouble(f);
		return *this;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(double f) {
		writeDouble(f);
		return *this;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(long double f) {
		writeDouble(f);
		return *this;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(const void* p) {
		writeUnsigned((unsigned long)(p));
		return *this;
	}

	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::operator <<(
			basic_streambuf<charT,traits>* sb) {
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
				basic_ios<charT,traits>::setstate(ios_base::badbit);
			}
			ios_base::width(0);
		}
		return *this;
	}

	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::put(char_type c) {
		return write(&c,1);
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::write(const char_type* s,streamsize n) {
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
				basic_ios<charT,traits>::setstate(ios_base::badbit);
			}
			ios_base::width(0);
		}
		return *this;
	}

	template<class charT,class traits>
	basic_ostream<charT,traits>& basic_ostream<charT,traits>::flush() {
		try {
			_sb->flush();
		}
		catch(...) {
			basic_ios<charT,traits>::setstate(ios_base::badbit);
		}
		return *this;
	}

	template<class charT,class traits>
	void basic_ostream<charT,traits>::writeSigned(signed long n) {
		// write as unsigned if oct or hex is desired
		if(!(ios_base::flags() & ios_base::dec)) {
			writeUnsigned(static_cast<unsigned long>(n));
			return;
		}
		sentry se(*this);
		if(se) {
			try {
				streamsize width = 0;
				streamsize pwidth = basic_ios<charT,traits>::width();
				// determine width
				if((ios_base::flags() & (ios_base::left | ios_base::right)) && pwidth > 0) {
					width = getnwidth(n);
					if(ios_base::flags() & ios_base::showpos)
						width++;
				}
				// pad left
				if((ios_base::flags() & ios_base::right) && pwidth > width)
					writePad(pwidth - width);
				// print '+' or ' ' instead of '-'
				if(ios_base::flags() & ios_base::showpos)
					_sb->put('+');
				// print number
				writeSChars(n);
				// pad right
				if((ios_base::flags() & ios_base::left) && pwidth > width)
					writePad(pwidth - width);
			}
			catch(...) {
				basic_ios<charT,traits>::setstate(ios_base::badbit);
			}
			ios_base::width(0);
		}
	}

	template<class charT,class traits>
	void basic_ostream<charT,traits>::writeUnsigned(unsigned long u) {
		static const char *numTableSmall = "0123456789abcdef";
		static const char *numTableBig = "0123456789ABCDEF";
		sentry se(*this);
		if(se) {
			try {
				int base = ios_base::get_base();
				// determine width
				streamsize width = 0;
				streamsize pwidth = basic_ios<charT,traits>::width();
				if((ios_base::flags() & (ios_base::left | ios_base::right)) && pwidth > 0) {
					width = getuwidth(u,base);
					if(u > 0 && (ios_base::flags() & ios_base::showbase)) {
						switch(base) {
							case 16:
								width += 2;
								break;
							case 8:
								width += 1;
								break;
						}
					}
				}
				// pad left - spaces
				if((ios_base::flags() & ios_base::right) && pwidth > width)
					writePad(pwidth - width);
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
				if((ios_base::flags() & ios_base::left) && pwidth > width)
					writePad(pwidth - width);
			}
			catch(...) {
				basic_ios<charT,traits>::setstate(ios_base::badbit);
			}
			ios_base::width(0);
		}
	}

	template<class charT,class traits>
	void basic_ostream<charT,traits>::writeDouble(long double d) {
		sentry se(*this);
		if(se) {
			try {
				long long pre = (long long)d;
				streamsize width = 0;
				streamsize pwidth = basic_ios<charT,traits>::width();
				if(!(ios_base::flags() & (ios_base::right | ios_base::left)) && pwidth > 0) {
					width = getlwidth(pre) + basic_ios<charT,traits>::precision() + 1;
					if(d >= 0 && (ios_base::flags() & ios_base::showpos))
						width++;
				}
				// pad left
				if((ios_base::flags() & ios_base::right) && pwidth > width)
					writePad(pwidth - width);
				if(d >= 0 && (ios_base::flags() & ios_base::showpos))
					_sb->put('+');
				// print number
				writeDoubleChars(d);
				// pad right
				if((ios_base::flags() & ios_base::left) && pwidth > width)
					writePad(pwidth - width);
			}
			catch(...) {
				basic_ios<charT,traits>::setstate(ios_base::badbit);
			}
			ios_base::width(0);
		}
	}

	template<class charT,class traits>
	void basic_ostream<charT,traits>::writeSChars(signed long n) {
		if(n < 0) {
			_sb->put('-');
			n = -n;
		}
		if(n >= 10)
			writeSChars(n / 10);
		_sb->put('0' + (n % 10));
	}

	template<class charT,class traits>
	void basic_ostream<charT,traits>::writeUChars(unsigned long u,int base,const char *hexchars) {
		if(u >= base)
			writeUChars(u / base,base,hexchars);
		_sb->put(hexchars[(u % base)]);
	}

	template<class charT,class traits>
	void basic_ostream<charT,traits>::writeDoubleChars(long double d) {
		long long val = 0;

		// Note: this is probably not a really good way of converting IEEE754-floating point numbers
		// to string. But its simple and should be enough for my purposes :)

		val = (long long)d;
		writeSChars(val);
		d -= val;
		if(d < 0)
			d = -d;
		_sb->put('.');
		streamsize precision = basic_ios<charT,traits>::precision();
		while(precision-- > 0) {
			d *= 10;
			val = (long long)d;
			_sb->put((val % 10) + '0');
			d -= val;
		}
	}

	template<class charT,class traits>
	void basic_ostream<charT,traits>::writePad(streamsize count) {
		char_type c = basic_ios<charT,traits>::fill();
		while(count-- > 0)
			_sb->put(c);
	}

	template<class charT,class traits>
	basic_ostream<charT,traits>& operator <<(basic_ostream<charT,traits>& os,charT c) {
		os.put(c);
		return os;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& operator <<(basic_ostream<charT,traits>& os,char c) {
		os.put(c);
		return os;
	}
	template<class traits>
	basic_ostream<char,traits>& operator <<(basic_ostream<char,traits>& os,char c) {
		os.put(c);
		return os;
	}

	template<class charT,class traits>
	basic_ostream<charT,traits>& operator <<(basic_ostream<charT,traits>& os,
			const basic_string<charT>& s) {
		basic_stringbuf<charT,traits> sb(s);
		os << &sb;
		return os;
	}

	template<class traits>
	basic_ostream<char,traits>& operator <<(basic_ostream<char,traits>& os,signed char c) {
		os.put(c);
		return os;
	}
	template<class traits>
	basic_ostream<char,traits>& operator <<(basic_ostream<char,traits>& os,unsigned char c) {
		os.put(c);
		return os;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& operator <<(basic_ostream<charT,traits>& os,const charT* s) {
		os.write(s,strlen(s));
		return os;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& operator <<(basic_ostream<charT,traits>& os,const char* s) {
		os.write(s,strlen(s));
		return os;
	}
	template<class traits>
	basic_ostream<char,traits>& operator <<(basic_ostream<char,traits>& os,const char* s) {
		os.write(s,strlen(s));
		return os;
	}

	template<class traits>
	basic_ostream<char,traits>& operator <<(basic_ostream<char,traits>& os,const signed char* s) {
		os.write(s,strlen(s));
		return os;
	}
	template<class traits>
	basic_ostream<char,traits>& operator <<(basic_ostream<char,traits>& os,const unsigned char* s) {
		os.write(s,strlen(s));
		return os;
	}

	template<class charT,class traits>
	basic_ostream<charT,traits>& endl(basic_ostream<charT,traits>& os) {
		os.put('\n');
		return os;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& ends(basic_ostream<charT,traits>& os) {
		os.put(charT());
		return os;
	}
	template<class charT,class traits>
	basic_ostream<charT,traits>& flush(basic_ostream<charT,traits>& os) {
		os.flush();
		return os;
	}
}
