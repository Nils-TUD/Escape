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

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <utility>
#include "iparser.h"
#include "idriver.h"

static pair<driver*,vector<string> > parseLine(const string& line);
static vector<string> split(const string& s,char delim);
static driver* getByName(const vector<driver*>& drvlist,const string& name);

vector<driver*> parseDrivers(const string& drivers) {
	vector<driver*> res;
	map<driver*,vector<string> > deps;
	string line;
	istringstream drvstr(drivers);
	while(getline(drvstr,line)) {
		if(line.size() > 0 && line[0] != '#') {
			// first store deps by name
			pair<driver*,vector<string> > driver = parseLine(line);
			deps[driver.first] = driver.second;
			res.push_back(driver.first);
		}
	}
	// resolve dependencies to driver-instances
	for(map<driver*,vector<string> >::iterator it = deps.begin(); it != deps.end(); ++it) {
		pair<driver*,vector<string> > drv = *it;
		for(vector<string>::iterator dit = drv.second.begin(); dit != drv.second.end(); ++dit) {
			driver* dep = getByName(res,*dit);
			if(!dep)
				throw load_error("Driver-depencency not found: '" + *dit + "'");
			drv.first->add_dep(dep);
		}
	}
	return res;
}

static pair<driver*,vector<string> > parseLine(const string& line) {
	vector<string> parts = split(line,';');
	if(parts.size() == 0 || parts.size() > 3)
		throw load_error("Invalid driver-spec; line has not 1-3 components: '" + line + "'");
	vector<string> waits;
	if(parts.size() > 1)
		waits = split(parts[1],',');
	driver *drv = new driver(parts[0],waits);
	vector<string> deps;
	if(parts.size() > 2)
		deps = split(parts[2],',');
	return make_pair<driver*,vector<string> >(drv,deps);
}

static vector<string> split(const string& s,char delim) {
	vector<string> tokens;
	istringstream is(s);
	string token;
	while(getline(is,token,delim)) {
		token.trim();
		tokens.push_back(token);
	}
	return tokens;
}

static driver* getByName(const vector<driver*>& drvlist,const string& name) {
	for(vector<driver*>::const_iterator it = drvlist.begin(); it != drvlist.end(); ++it) {
		if((*it)->name() == name)
			return *it;
	}
	return NULL;
}
