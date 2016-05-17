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

#include <esc/stream/istream.h>
#include <esc/stream/ostream.h>
#include <sys/common.h>
#include <usergroup/usergroup.h>
#include <string>
#include <vector>

#include "process.h"

class Device {
	friend esc::IStream& operator >>(esc::IStream& is,Device& dev);
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
	friend esc::IStream& operator >>(esc::IStream& is,DriverProcess& drv);

public:
	static const int MAX_WAIT_RETRIES	= 1000;
	static const int RETRY_INTERVAL		= 40000 /* us */;

private:
	static sNamedItem *groupList;

public:
	DriverProcess() : Process(), _devices() {
	}
	virtual ~DriverProcess() {
	}

	virtual bool isKillable() const {
		return name() != "video";
	}
	const std::vector<Device>& devices() const {
		return _devices;
	}
	virtual void load();

private:
	std::vector<Device> _devices;
};

esc::IStream& operator >>(esc::IStream& is,Device& dev);
esc::IStream& operator >>(esc::IStream& is,DriverProcess& drv);
esc::OStream& operator <<(esc::OStream& os,const DriverProcess& drv);
