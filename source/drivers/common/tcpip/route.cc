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

#include <esc/common.h>

#include "route.h"

std::vector<Route*> Route::_table;

int Route::insert(const ipc::Net::IPv4Addr &dest,const ipc::Net::IPv4Addr &nm,
		const ipc::Net::IPv4Addr &gw,uint flags,Link *link) {
	// if we should use the gateway, it has to be a valid host
	if((flags & ipc::Net::FL_USE_GW) && !gw.isHost(nm))
		return -EINVAL;
	if(!nm.isNetmask() || !link)
		return -EINVAL;

	auto it = _table.begin();
	for(; it != _table.end(); ++it) {
		if(nm >= (*it)->netmask)
			break;
	}
	_table.insert(it,new Route(dest,nm,gw,flags,link));
	return 0;
}

const Route *Route::find(const ipc::Net::IPv4Addr &ip) {
	for(auto it = _table.begin(); it != _table.end(); ++it) {
		if(((*it)->flags & ipc::Net::FL_UP) && (*it)->dest.sameNetwork(ip,(*it)->netmask))
			return *it;
	}
	return NULL;
}

int Route::setStatus(const ipc::Net::IPv4Addr &ip,ipc::Net::Status status) {
	for(auto it = _table.begin(); it != _table.end(); ++it) {
		if((*it)->dest == ip) {
			if(status == ipc::Net::DOWN)
				(*it)->flags &= ~ipc::Net::FL_UP;
			else
				(*it)->flags |= ipc::Net::FL_UP;
			return 0;
		}
	}
	return -ENOENT;
}

int Route::remove(const ipc::Net::IPv4Addr &ip) {
	for(auto it = _table.begin(); it != _table.end(); ++it) {
		if((*it)->dest == ip) {
			_table.erase(it);
			return 0;
		}
	}
	return -ENOENT;
}

void Route::print(std::ostream &os) {
	for(auto it = _table.begin(); it != _table.end(); ++it) {
		os << (*it)->dest << " " << (*it)->gateway << " " << (*it)->netmask << " ";
		os << (*it)->flags << " " << (*it)->link->name() << "\n";
	}
}
