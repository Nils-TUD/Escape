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

#pragma once

#include <exception>
#include <sstream>

/**
 * This macro throws an exception and passes a formatted string as its message. That is, you can
 * use the stream operators to build the message. For example:
 * VTHROW("My exception " << 1 << "," << 2 << " message");
 * VTHROWE(My exception " << 1 << "," << 2 << " message",-EINVAL);
 */
#define VTHROW(expr) {										\
		std::ostringstream __os;							\
		__os << expr;										\
		throw std::default_error(__os.str());				\
	}
#define VTHROWE(expr,errorcode) {							\
		std::ostringstream __os;							\
		__os << expr;										\
		throw std::default_error(__os.str(),errorcode);		\
	}

namespace std {

/**
 * The default error class with a string and an error-code
 */
class default_error : public exception {
public:
	explicit default_error(const string& s,int err = 0)
		: _error(err), _msg(s) {
		if(err != 0)
			_msg = _msg + ": " + strerror(err);
	}
	virtual ~default_error() throw() {
	}

	int error() const throw() {
		return _error;
	}
	virtual const char* what() const throw() {
		return _msg.c_str();
	}
private:
	int _error;
	string _msg;
};

}
