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

#include <esc/env.h>
#include <stdlib.h>

namespace esc {
	std::map<std::string,std::string> env::list() {
		std::map<std::string,std::string> res;
		for(int i = 0; ; ++i) {
			char *name = getenvi(i);
			if(name == nullptr)
				break;
			res[name] = get(name);
		}
		return res;
	}
	std::string& env::absolutify(std::string& path) {
		if(path.size() == 0 || path[0] != '/') {
			std::string cwd = env::get("CWD");
			if(cwd.compare(cwd.length() - 1,1,"/") != 0)
				cwd += '/';
			path.insert(0,cwd);
		}
		return path;
	}
	std::string env::get(const std::string& name) {
		char *val = getenv(name.c_str());
		if(!val)
			throw default_error(std::string("Unable to get value of ") + name,errno);
		return std::string(val);
	}
	void env::set(const std::string& name,const std::string& value) {
		if(setenv(name.c_str(),value.c_str()) < 0)
			throw default_error("Unable to set env-variable",errno);
	}
}
