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

#include <ostream>
#include <sys/width.h>
#include <string.h>

#define FFL_SHORT			1
#define FFL_LONG			2
#define FFL_LONGLONG		4

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

	ostream& ostream::operator <<(bool n) {
		if(ios_base::flags() & ios_base::boolalpha)
			write(n ? "true" : "false",n ? SSTRLEN("true") : SSTRLEN("false"));
		else
			writeUnsigned(n);
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
				catch(...) {
					ios::setstate(ios_base::badbit);
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

	void ostream::writeUPadLeft(unsigned long long u,int base,streamsize &nwidth,streamsize &pwidth) {
		// determine width
		nwidth = 0;
		pwidth = ios::width();
		if((ios_base::flags() & (ios_base::left | ios_base::right)) && pwidth > 0) {
			nwidth = getullwidth(u,base);
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
	}

	void ostream::writePadLeft(long long n,streamsize &nwidth,streamsize &pwidth) {
		nwidth = 0;
		pwidth = ios::width();
		// determine width
		if((ios_base::flags() & (ios_base::left | ios_base::right)) && pwidth > 0) {
			nwidth = getllwidth(n);
			if(ios_base::flags() & ios_base::showpos)
				nwidth++;
		}
		// pad left
		if((ios_base::flags() & ios_base::right) && pwidth > nwidth)
			writePad(pwidth - nwidth);
		// print '+' or ' ' instead of '-'
		if(ios_base::flags() & ios_base::showpos)
			_sb->put('+');
	}

	void ostream::writeDouble(long double d) {
		sentry se(*this);
		if(se) {
			try {
				long long pre = (long long)d;
				streamsize nwidth = 0;
				streamsize pwidth = ios::width();
				if((ios_base::flags() & (ios_base::right | ios_base::left)) && pwidth > 0) {
					nwidth = getllwidth(pre) + ios::precision() + 1;
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
}
