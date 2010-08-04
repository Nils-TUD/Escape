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

#include <streambuf>

namespace esc {
	eof_reached::eof_reached() {
	}
	const char* eof_reached::what() const throw() {
		return "EOF reached";
	}

	bad_state::bad_state(const string &msg) : _msg(msg.c_str()) {
	}
	const char* bad_state::what() const throw() {
		return _msg;
	}

	streambuf::streambuf() {
	}
	streambuf::~streambuf() {
	}
}
