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

#include <info/arp.h>
#include <assert.h>
#include <fstream>

using namespace std;

namespace info {
	std::vector<arp*> arp::get_list() {
		std::vector<arp*> list;
		ifstream f("/sys/net/arp");
		while(f.good()) {
			arp *a = new arp;
			f >> *a;
			if(!f.good()) {
				delete a;
				break;
			}
			list.push_back(a);
		}
		return list;
	}

	istream& operator >>(istream& is,arp& a) {
		istream::size_type unlimited = numeric_limits<streamsize>::max();
		is >> a._ip >> a._mac;
		is.ignore(unlimited,'\n');
		return is;
	}

	std::ostream& operator <<(std::ostream& os,const arp& a) {
		os << "ARP[ip=" << a.ip() << ", mac=" << a.mac() << "]";
		return os;
	}
}
