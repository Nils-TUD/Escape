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
	template<class charT,class traits>
	basic_ostringstream<charT,traits>::basic_ostringstream(basic_string<charT>& str,
			ios_base::openmode which)
		: basic_ostream<charT,traits>(new basic_stringbuf<charT,traits>(str,which)) {
	}
	template<class charT,class traits>
	basic_ostringstream<charT,traits>::~basic_ostringstream() {
	}

	template<class charT,class traits>
	basic_stringbuf<charT,traits>* basic_ostringstream<charT,traits>::rdbuf() const {
		return static_cast<basic_stringbuf<charT,traits>*>(basic_ios<charT,traits>::rdbuf());
	}
	template<class charT,class traits>
	basic_string<charT>& basic_ostringstream<charT,traits>::str() const {
		basic_stringbuf<charT,traits>* buf = rdbuf();
		return buf->str();
	}
	template<class charT,class traits>
	void basic_ostringstream<charT,traits>::str(basic_string<charT>& s) {
		basic_stringbuf<charT,traits>* buf = rdbuf();
		buf->str(s);
	}
}
