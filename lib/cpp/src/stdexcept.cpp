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

#include <stddef.h>
#include <stdexcept>

namespace std {
	logic_error::logic_error(const string& msg)
		: _M_msg(msg) {
	}
	logic_error::~logic_error() throw() {
	}
	const char* logic_error::what() const throw() {
		return _M_msg.c_str();
	}

	domain_error::domain_error(const string& msg)
		: logic_error(msg) {
	}

	invalid_argument::invalid_argument(const string& msg)
		: logic_error(msg) {
	}

	length_error::length_error(const string& msg)
		: logic_error(msg) {
	}

	out_of_range::out_of_range(const string& msg)
		: logic_error(msg) {
	}

	runtime_error::runtime_error(const string& msg)
		: _M_msg(msg) {
	}
	runtime_error::~runtime_error() {
	}
	const char* runtime_error::what() const throw() {
		return _M_msg.c_str();
	}

	range_error::range_error(const string& msg)
		: runtime_error(msg) {
	}

	overflow_error::overflow_error(const string& msg)
		: runtime_error(msg) {
	}

	underflow_error::underflow_error(const string& msg)
		: runtime_error(msg) {
	}
}
