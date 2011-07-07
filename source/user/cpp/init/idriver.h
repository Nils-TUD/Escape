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

#ifndef DRIVER_H_
#define DRIVER_H_

#include <esc/common.h>
#include <usergroup/group.h>
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

class device {
	friend std::istream& operator >>(std::istream& is,device& dev);
	typedef unsigned int perm_type;

public:
	device(): _name(std::string()), _perms(perm_type()), _group(std::string()) {
	}
	~device() {
	}

	const std::string& name() const {
		return _name;
	}
	perm_type permissions() const {
		return _perms;
	}
	const std::string& group() const {
		return _group;
	}

private:
	std::string _name;
	perm_type _perms;
	std::string _group;
};

class driver {
	friend std::istream& operator >>(std::istream& is,driver& drv);

public:
	static const int MAX_WAIT_RETRIES	= 1000;
	static const int RETRY_INTERVAL		= 40;

private:
	static sGroup *groupList;

public:
	driver() : _name(std::string()), _devices(std::vector<device>()) {
	}
	~driver() {
	}

	const std::string& name() const {
		return _name;
	}
	const std::vector<device>& devices() const {
		return _devices;
	}
	void load();

private:
	std::string _name;
	std::vector<device> _devices;
};

std::istream& operator >>(std::istream& is,device& dev);
std::istream& operator >>(std::istream& is,driver& drv);
std::ostream& operator <<(std::ostream& os,const driver& drv);

#endif /* DRIVER_H_ */
