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

#include <esc/common.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/io.h>
#include <iostream>
#include "idriver.h"

void driver::load() {
	if(loaded())
		return;

	// load dependencies
	for(std::vector<driver*>::const_iterator it = _deps.begin(); it != _deps.end(); ++it)
		(*it)->load();

	// now load the driver
	std::string path = string("/sbin/") + _name;
	s32 child = fork();
	if(child == 0) {
		exec(path.c_str(),NULL);
		cerr << "Exec of '" << path << "' failed" << endl;
		exit(EXIT_FAILURE);
	}
	else if(child < 0)
		throw load_error("fork failed");

	// wait for all specified waits
	for(std::vector<std::string>::const_iterator it = _waits.begin(); it != _waits.end(); ++it) {
		sFileInfo info;
		s32 res;
		int j = 0;
		do {
			res = stat(it->c_str(),&info);
			if(res < 0)
				sleep(10);
		}
		while(j++ < MAX_WAIT_RETRIES && res < 0);
		if(res < 0)
			throw load_error(string("Max retried reached while waiting for '") + *it + "'");
	}

	_loaded = true;
}

std::ostream& operator <<(std::ostream& os,const driver& drv) {
	os << drv.name() << '\n';
	os << "	deps: ";
	const std::vector<driver*>& deps = drv.dependencies();
	for(std::vector<driver*>::const_iterator it = deps.begin(); it != deps.end(); ++it) {
		os << (*it)->name();
		if(it + 1 != deps.end())
			os << ", ";
	}
	os << '\n';
	os << "	waits: ";
	const std::vector<std::string>& waits = drv.waits();
	for(std::vector<std::string>::const_iterator it = waits.begin(); it != waits.end(); ++it) {
		os << *it;
		if(it + 1 != waits.end())
			os << ", ";
	}
	os << endl;
	return os;
}
