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

#pragma once

#include <esc/common.h>
#include <usergroup/group.h>
#include <string>
#include <vector>
#include <ostream>
#include "process.h"

class Device {
	friend std::istream& operator >>(std::istream& is,Device& dev);
	typedef unsigned int perm_type;

public:
	Device() : _name(), _perms(), _group() {
	}
	~Device() {
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

class DriverProcess : public Process {
	friend std::istream& operator >>(std::istream& is,DriverProcess& drv);

public:
	static const int MAX_WAIT_RETRIES	= 100;
	static const int RETRY_INTERVAL		= 40;

private:
	static sGroup *groupList;

public:
	DriverProcess() : Process(), _devices() {
	}
	virtual ~DriverProcess() {
	}

	virtual bool isKillable() const {
		return name() != "vga";
	}
	const std::vector<Device>& devices() const {
		return _devices;
	}
	virtual void load();

private:
	std::vector<Device> _devices;
};

std::istream& operator >>(std::istream& is,Device& dev);
std::istream& operator >>(std::istream& is,DriverProcess& drv);
std::ostream& operator <<(std::ostream& os,const DriverProcess& drv);
