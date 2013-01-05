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

#include <impl/streams/ios_base.h>
#include <iostream>
#include <fstream>
#include <esc/debug.h>
#include <stdio.h>
#include <string.h>

namespace std {
	extern ifstream _cin;
	extern ofstream _cout;
	extern ofstream _cerr;
	extern ofstream _clog;

	int ios_base::Init::init_cnt = 0;

	ios_base::Init::Init() {
		if(init_cnt++ == 0) {
			_cin.open((int)STDIN_FILENO);
			_cin.tie(&cout);
			_cout.open((int)STDOUT_FILENO);
			_cerr.open((int)STDERR_FILENO);
			_cerr.tie(&cout);
			_clog.open((int)STDERR_FILENO);
		}
	}
	ios_base::Init::~Init() {
		if(--init_cnt == 0) {
			cout.flush();
			cerr.flush();
			clog.flush();
		}
	}

	const ios_base::fmtflags ios_base::boolalpha	= 1 << 0;
	const ios_base::fmtflags ios_base::dec			= 1 << 1;
	const ios_base::fmtflags ios_base::fixed		= 1 << 2;
	const ios_base::fmtflags ios_base::hex			= 1 << 3;
	const ios_base::fmtflags ios_base::internal		= 1 << 4;
	const ios_base::fmtflags ios_base::left			= 1 << 5;
	const ios_base::fmtflags ios_base::oct			= 1 << 6;
	const ios_base::fmtflags ios_base::right		= 1 << 7;
	const ios_base::fmtflags ios_base::scientific	= 1 << 8;
	const ios_base::fmtflags ios_base::showbase		= 1 << 9;
	const ios_base::fmtflags ios_base::showpoint	= 1 << 10;
	const ios_base::fmtflags ios_base::showpos		= 1 << 11;
	const ios_base::fmtflags ios_base::skipws		= 1 << 12;
	const ios_base::fmtflags ios_base::unitbuf		= 1 << 13;
	const ios_base::fmtflags ios_base::uppercase	= 1 << 14;

	const ios_base::fmtflags ios_base::adjustfield	= left | right | internal;
	const ios_base::fmtflags ios_base::basefield	= dec | oct | hex;
	const ios_base::fmtflags ios_base::floatfield	= scientific | fixed;

	const ios_base::iostate ios_base::goodbit		= 0;
	const ios_base::iostate ios_base::badbit		= 1 << 0;
	const ios_base::iostate ios_base::eofbit		= 1 << 1;
	const ios_base::iostate ios_base::failbit		= 1 << 2;

	const ios_base::openmode ios_base::app			= 1 << 0;
	const ios_base::openmode ios_base::ate			= 1 << 1;
	const ios_base::openmode ios_base::binary		= 1 << 2;
	const ios_base::openmode ios_base::in			= 1 << 3;
	const ios_base::openmode ios_base::out			= 1 << 4;
	const ios_base::openmode ios_base::trunc		= 1 << 5;
	const ios_base::openmode ios_base::noblock		= 1 << 6;

	const ios_base::seekdir ios_base::beg			= SEEK_SET;
	const ios_base::seekdir ios_base::cur			= SEEK_CUR;
	const ios_base::seekdir ios_base::end			= SEEK_END;

	ios_base::ios_base()
		: _flags(), _prec(), _width(), _callbacks() {
		// do nothing here
	}

	void ios_base::raise_event(event ev) {
		for(auto it = _callbacks.rbegin(); it != _callbacks.rend(); ++it)
			it->first(ev,*this,it->second);
	}

	int ios_base::get_base() {
		int base = 10;
		if(flags() & oct)
			base = 8;
		else if(flags() & hex)
			base = 16;
		return base;
	}
}
