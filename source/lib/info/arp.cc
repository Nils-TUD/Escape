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

#include <esc/stream/fstream.h>
#include <info/arp.h>
#include <assert.h>

using namespace esc;

namespace info {
	std::vector<arp*> arp::get_list() {
		std::vector<arp*> list;
		FStream f("/sys/net/arp","r");
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

	IStream& operator >>(IStream& is,arp& a) {
		size_t unlimited = std::numeric_limits<size_t>::max();
		is >> a._ip >> a._mac;
		is.ignore(unlimited,'\n');
		return is;
	}

	OStream& operator <<(OStream& os,const arp& a) {
		os << "ARP[ip=" << a.ip() << ", mac=" << a.mac() << "]";
		return os;
	}
}
