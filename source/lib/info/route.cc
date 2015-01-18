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

#include <info/route.h>
#include <assert.h>
#include <fstream>

using namespace std;

namespace info {
	std::vector<route*> route::get_list() {
		std::vector<route*> list;
		ifstream f("/sys/net/routes");
		while(f.good()) {
			route *r = new route;
			f >> *r;
			if(!f.good()) {
				delete r;
				break;
			}
			list.push_back(r);
		}
		return list;
	}

	istream& operator >>(istream& is,route& r) {
		is >> r._dest >> r._gw >> r._subnetmask >> r._flags;
		is.getline(r._link,'\n');
		return is;
	}

	std::ostream& operator <<(std::ostream& os,const route& r) {
		os << "Route[dest=" << r.dest() << ", netmask=" << r.subnetMask()
		   << ", gateway=" << r.gateway() << ", flags=" << r.flags() << ", link=" << r.link() << "]";
		return os;
	}
}
