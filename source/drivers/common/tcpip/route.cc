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

const Route *Route::find(const IPv4Addr &ip) {
	for(auto it = _table.begin(); it != _table.end(); ++it) {
		if(((*it)->flags & FL_UP) && (*it)->dest.sameNetwork(ip,(*it)->netmask))
			return *it;
	}
	return NULL;
}

int Route::insert(const IPv4Addr &dest,const IPv4Addr &nm,const IPv4Addr &gw,uint flags,NIC *nic) {
	// if we should use the gateway, it has to be a valid host
	if((flags & FL_USE_GW) && !gw.isHost(nm))
		return -EINVAL;
	if(!nm.isNetmask() || !nic)
		return -EINVAL;

	auto it = _table.begin();
	for(; it != _table.end(); ++it) {
		if(nm >= (*it)->netmask)
			break;
	}
	_table.insert(it,new Route(dest,nm,gw,flags,nic));
	return 0;
}
