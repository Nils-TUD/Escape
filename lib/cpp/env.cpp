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

#include <env.h>
#include <stdlib.h>

namespace std {
	map<string,string> env::list() {
		map<string,string> res;
		for(int i = 0; ; ++i) {
			char *name = getenvi(i);
			if(name == NULL)
				break;
			res[name] = get(name);
		}
		return res;
	}
	string env::get(const string& name) {
		char *val = getenv(name.c_str());
		if(!val)
			throw io_exception(string("Unable to get value of ") + name,errno);
		return string(val);
	}
	void env::set(const string& name,const string& value) {
		if(setenv(name.c_str(),value.c_str()) < 0)
			throw io_exception("Unable to set env-variable",errno);
	}
}
