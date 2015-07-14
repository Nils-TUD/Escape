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

#include <esc/stream/std.h>
#include <sys/common.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <sys/io.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/thread.h>
#include <usergroup/group.h>

#include "../initerror.h"
#include "driverprocess.h"

using namespace std;

sGroup *DriverProcess::groupList = nullptr;

void DriverProcess::load() {
	// load groups from file, if not already done
	if(groupList == nullptr) {
		size_t count;
		groupList = group_parseFromFile(GROUPS_PATH,&count);
		if(!groupList)
			throw init_error("Unable to load groups from file");
	}

	// now load the driver
	_pid = fork();
	if(_pid == 0) {
		// keep only stdin, stdout and stderr
		int maxfds = sysconf(CONF_MAX_FDS);
		for(int i = 3; i < maxfds; ++i)
			close(i);

		// build args and exec
		const char **argv = new const char*[_args.size() + 2];
		size_t idx = 0;
		argv[idx++] = name().c_str();
		for(size_t i = 0; i < _args.size(); i++) {
			// if there is a '=' we should set an environment variable
			int pos = strchri(_args[i].c_str(),'=');
			if((size_t)pos == strlen(_args[i].c_str()))
				argv[idx++] = _args[i].c_str();
			else {
				string tmp(_args[i].c_str(),pos);
				setenv(tmp.c_str(),_args[i].c_str() + pos + 1);
			}
		}
		argv[idx++] = nullptr;
		execvpe(argv[0],argv,(const char**)environ);

		// if we're here, there's something wrong
		esc::serr << "Exec of '" << name() << "' failed" << esc::endl;
		exit(EXIT_FAILURE);
	}
	else if(_pid < 0)
		throw init_error("fork failed");

	// wait for all specified devices
	for(auto it = _devices.begin(); it != _devices.end(); ++it) {
		struct stat info;
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
			throw init_error(string("Max retries reached while waiting for '") + it->name() + "'");

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

esc::IStream& operator >>(esc::IStream& is,Device& dev) {
	is >> dev._name;
	is >> esc::fmt(dev._perms,"o");
	is >> dev._group;
	return is;
}

esc::IStream& operator >>(esc::IStream& is,DriverProcess& drv) {
	char c;
	is >> drv._name;
	while(is.good() && (c = is.get()) != '\t') {
		is.putback(c);
		string arg;
		is >> arg;
		drv._args.push_back(arg);
	}
	if(is.good()) {
		is.putback(c);
		while((c = is.get()) == '\t') {
			is.putback(c);
			Device dev;
			is >> dev;
			drv._devices.push_back(dev);
		}
		is.putback(c);
	}
	return is;
}

esc::OStream& operator <<(esc::OStream& os,const DriverProcess& drv) {
	os << drv.name() << '\n';
	const vector<Device>& devs = drv.devices();
	for(auto it = devs.begin(); it != devs.end(); ++it) {
		os << '\t' << it->name() << ' ';
		os << esc::fmt(it->permissions(),"0o",4) << ' ' << it->group() << '\n';
	}
	os << esc::endl;
	return os;
}
