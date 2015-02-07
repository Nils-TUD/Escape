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
#include <info/link.h>
#include <assert.h>

using namespace esc;

namespace info {
	std::vector<link*> link::get_list() {
		std::vector<link*> list;
		FStream f("/sys/net/links","r");
		while(f.good()) {
			link *l = new link;
			f >> *l;
			if(!f.good()) {
				delete l;
				break;
			}
			list.push_back(l);
		}
		return list;
	}

	IStream& operator >>(IStream& is,link& l) {
		size_t unlimited = std::numeric_limits<size_t>::max();
		uint status = 0;
		is >> l._name >> status >> l._mac >> l._ip >> l._subnetmask >> l._mtu;
		is >> l._rxpkts >> l._txpkts >> l._rxbytes >> l._txbytes;
		is.ignore(unlimited,'\n');
		l._status = static_cast<esc::Net::Status>(status);
		return is;
	}

	OStream& operator <<(OStream& os,const link& l) {
		os << "Link[name=" << l.name() << ", MAC=" << l.mac()
		   << ", ip=" << l.ip() << ", netmask=" << l.subnetMask() << "]";
		return os;
	}
}
