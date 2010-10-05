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
	template<class T>
	void ostream::writeSigned(T n) {
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

	template<class T>
	void ostream::writeUnsigned(T u) {
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

	template<class T>
	void ostream::writeSChars(T n) {
		if(n < 0) {
			_sb->put('-');
			n = -n;
		}
		if(n >= 10)
			writeSChars(n / 10);
		_sb->put('0' + (n % 10));
	}

	template<class T>
	void ostream::writeUChars(T u,unsigned int base,const char *hexchars) {
		if(u >= base)
			writeUChars(u / base,base,hexchars);
		_sb->put(hexchars[(u % base)]);
	}
}
