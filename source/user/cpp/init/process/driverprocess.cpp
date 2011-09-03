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

#include <esc/common.h>
#include <usergroup/group.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/debug.h>
#include <esc/io.h>
#include <iostream>

#include "driverprocess.h"
#include "../initerror.h"

sGroup *DriverProcess::groupList = NULL;

void DriverProcess::load() {
	// load groups from file, if not already done
	if(groupList == NULL) {
		size_t count;
		groupList = group_parseFromFile(GROUPS_PATH,&count);
		if(!groupList)
			throw init_error("Unable to load groups from file");
	}

	// now load the driver
	std::string path = string("/sbin/") + name();
	_pid = fork();
	if(_pid == 0) {
		exec(path.c_str(),NULL);
		cerr << "Exec of '" << path << "' failed" << endl;
		exit(EXIT_FAILURE);
	}
	else if(_pid < 0)
		throw init_error("fork failed");

	// wait for all specified devices
	for(std::vector<Device>::const_iterator it = _devices.begin(); it != _devices.end(); ++it) {
		sFileInfo info;
		sGroup *g;
		int res;
		int j = 0;
		do {
			res = stat(it->name().c_str(),&info);
			if(res < 0)
				sleep(RETRY_INTERVAL);
		}
		while(j++ < MAX_WAIT_RETRIES && res < 0);
		if(res < 0)
			throw init_error(string("Max retried reached while waiting for '") + it->name() + "'");

		// set permissions
		if(chmod(it->name().c_str(),it->permissions()) < 0)
			throw init_error(string("Unable to set permissions for '") + it->name() + "'");
		// set group
		g = group_getByName(groupList,it->group().c_str());
		if(!g)
			throw init_error(string("Unable to find group '") + it->group() + "'");
		if(chown(it->name().c_str(),-1,g->gid) < 0)
			throw init_error(string("Unable to set group for '") + it->name() + "'");
	}
}

std::istream& operator >>(std::istream& is,Device& dev) {
	is >> dev._name;
	is >> std::oct >> dev._perms;
	is >> dev._group;
	return is;
}

std::istream& operator >>(std::istream& is,DriverProcess& drv) {
	is >> drv._name;
	while(is.peek() == '\t') {
		Device dev;
		is >> dev;
		drv._devices.push_back(dev);
	}
	return is;
}

std::ostream& operator <<(std::ostream& os,const DriverProcess& drv) {
	os << drv.name() << '\n';
	const std::vector<Device>& devs = drv.devices();
	for(std::vector<Device>::const_iterator it = devs.begin(); it != devs.end(); ++it) {
		os << '\t' << it->name() << ' ';
		os << std::oct << it->permissions() << ' ' << it->group() << '\n';
	}
	os << std::endl;
	return os;
}
