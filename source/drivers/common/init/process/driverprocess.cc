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

#include <esc/stream/istringstream.h>
#include <esc/stream/std.h>
#include <esc/vthrow.h>
#include <sys/common.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <sys/io.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/thread.h>
#include <sys/wait.h>
#include <usergroup/usergroup.h>

#include "driverprocess.h"

using namespace std;

sNamedItem *DriverProcess::groupList = nullptr;

void DriverProcess::load() {
	// load groups from file, if not already done
	if(groupList == nullptr) {
		size_t count;
		groupList = usergroup_parse(GROUPS_PATH,&count);
		if(!groupList)
			throw esc::default_error("Unable to get group list");
	}

	// now load the driver
	_pid = fork();
	if(_pid == 0) {
		// keep only stdin, stdout and stderr and strace
		int maxfds = sysconf(CONF_MAX_FDS);
		for(int i = 4; i < maxfds; ++i)
			close(i);

		// change to specified user
		if(usergroup_changeToName(_user.c_str()) < 0)
			throw esc::default_error(string("Changing to user '") + _user + "' failed");

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
		throw esc::default_error("fork failed");

	// wait for all specified devices
	for(auto it = _devices.begin(); it != _devices.end(); ++it) {
		sNamedItem *g;
		int fd;
		int j = 0;
		do {
			fd = open(it->name().c_str(),O_NOCHAN);
			if(fd < 0) {
				// check whether the child already died
				int state;
				if(waitpid(_pid,&state,WNOHANG) == _pid) {
					VTHROW("Child " << _pid << ":" << name() << " died with exitcode "
						<< WEXITSTATUS(state) << " (signal " << WTERMSIG(state) << ")");
				}

				usleep(RETRY_INTERVAL);
			}
		}
		while(j++ < MAX_WAIT_RETRIES && fd < 0);
		if(fd < 0)
			throw esc::default_error(string("Max retries reached while waiting for '") + it->name() + "'");

		// set permissions
		if(fchmod(fd,it->permissions()) < 0)
			throw esc::default_error(string("Unable to set permissions for '") + it->name() + "'");
		// set group
		g = usergroup_getByName(groupList,it->group().c_str());
		if(!g)
			throw esc::default_error(string("Unable to find group '") + it->group() + "'");
		if(fchown(fd,-1,g->id) < 0)
			throw esc::default_error(string("Unable to set group for '") + it->name() + "'");
		close(fd);
	}
}

esc::IStream& operator >>(esc::IStream& is,Device& dev) {
	is >> dev._name;
	is >> esc::fmt(dev._perms,"o");
	is >> dev._group;
	return is;
}

esc::IStream& operator >>(esc::IStream& is,DriverProcess& drv) {
	/* read name and args */
	std::string line;
	is.getline(line);
	esc::IStringStream tmp(line);
	tmp >> drv._user >> drv._name;
	while(!tmp.eof()) {
		string arg;
		tmp >> arg;
		drv._args.push_back(arg);
	}

	/* read devices, if any */
	if(is.good()) {
		char c;
		while((c = is.get()) == '\t') {
			Device dev;
			is >> dev;
			drv._devices.push_back(dev);
		}
		is.putback(c);
	}
	return is;
}

esc::OStream& operator <<(esc::OStream& os,const DriverProcess& drv) {
	os << drv.name() << " [with user " << drv.user() << "]\n";
	const vector<Device>& devs = drv.devices();
	for(auto it = devs.begin(); it != devs.end(); ++it) {
		os << '\t' << it->name() << ' ';
		os << esc::fmt(it->permissions(),"0o",4) << ' ' << it->group() << '\n';
	}
	os << esc::endl;
	return os;
}
