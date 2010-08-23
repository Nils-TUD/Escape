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

#ifndef DRIVER_H_
#define DRIVER_H_

#include <esc/common.h>
#include <string>
#include <vector>
#include <ostream>

class load_error : public std::exception {
public:
	explicit load_error(const std::string& error)
		: _msg(error) {
	}
	virtual ~load_error() throw () {
	}

	virtual const char* what() const throw () {
		return _msg.c_str();
	}
private:
	std::string _msg;
};

class driver {
public:
	static const int MAX_WAIT_RETRIES = 1000;

public:
	driver(const std::string& n,const std::vector<std::string>& waitNames)
		: _loaded(false), _name(n), _deps(vector<driver*>()), _waits(waitNames) {
	}
	~driver() {
	}

	bool loaded() const {
		return _loaded;
	}
	const std::string& name() const {
		return _name;
	}
	const std::vector<driver*>& dependencies() const {
		return _deps;
	}
	const std::vector<std::string>& waits() const {
		return _waits;
	}

	void load();
	void add_dep(driver* drv) {
		_deps.push_back(drv);
	}

private:
	bool _loaded;
	std::string _name;
	std::vector<driver*> _deps;
	std::vector<std::string> _waits;
};

std::ostream& operator <<(std::ostream& os,const driver& drv);

#endif /* DRIVER_H_ */
