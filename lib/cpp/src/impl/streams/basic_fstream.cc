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
	basic_fstream<charT,traits>::basic_fstream()
		: basic_iostream<charT,traits>(new basic_filebuf<charT,traits>()) {
	}
	template<class charT,class traits>
	basic_fstream<charT,traits>::basic_fstream(const char* filename,ios_base::openmode which)
		: basic_iostream<charT,traits>(new basic_filebuf<charT,traits>()) {
		rdbuf()->open(filename,which);
	}
	template<class charT,class traits>
	basic_fstream<charT,traits>::~basic_fstream() {
	}

	template<class charT,class traits>
	basic_filebuf<charT,traits>* basic_fstream<charT,traits>::rdbuf() const {
		return static_cast<basic_filebuf<charT,traits>*>(basic_ios<charT,traits>::rdbuf());
	}
	template<class charT,class traits>
	void basic_fstream<charT,traits>::open(const char* s,ios_base::openmode mode) {
		rdbuf()->open(s,mode);
	}
	template<class charT,class traits>
	bool basic_fstream<charT,traits>::is_open() const {
		return rdbuf()->is_open();
	}
	template<class charT,class traits>
	void basic_fstream<charT,traits>::close() {
		rdbuf()->close();
	}
}
