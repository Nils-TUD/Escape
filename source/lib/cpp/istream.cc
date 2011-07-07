/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
	void istream::readInteger(T &n,bool sign) {
		sentry se(*this,false);
		if(se) {
			bool neg = false;
			try {
				static const char *numTable = "0123456789abcdef";
				int base = ios_base::get_base();

				// handle '-'
				if(sign) {
					char_type rc = _sb->peek();
					if(rc == EOF) {
						ios::setstate(ios_base::eofbit);
						goto done;
					}
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
					char_type tc = _sb->get();
					if(tc == EOF) {
						ios::setstate(ios_base::eofbit);
						break;
					}
					tc = tolower(tc);
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
			catch(...) {
				ios::setstate(ios_base::badbit);
			}
			done:
			if(neg)
				n = -n;
			ios_base::width(0);
		}
	}
}
