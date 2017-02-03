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

#include <sys/common.h>

#include "route.h"

std::mutex Route::_mutex;
std::vector<Route*> Route::_table;

int Route::insert(const esc::Net::IPv4Addr &dest,const esc::Net::IPv4Addr &nm,
		const esc::Net::IPv4Addr &gw,uint flags,const std::shared_ptr<Link> &l) {
	// if we should use the gateway, it has to be a valid host
	if((flags & esc::Net::FL_USE_GW) && (gw.value() == 0 || !gw.isHost(nm)))
		return -EINVAL;
	if(!nm.isNetmask() || !l)
		return -EINVAL;

	std::lock_guard<std::mutex> guard(_mutex);
	auto it = _table.begin();
	for(; it != _table.end(); ++it) {
		if(nm >= (*it)->netmask)
			break;
	}
	_table.insert(it,new Route(dest,nm,gw,flags,l));
	return 0;
}

Route Route::find(const esc::Net::IPv4Addr &ip) {
	std::lock_guard<std::mutex> guard(_mutex);
	for(auto it = _table.begin(); it != _table.end(); ++it) {
		if(((*it)->flags & esc::Net::FL_UP) && (*it)->dest.sameNetwork(ip,(*it)->netmask))
			return **it;
	}
	return Route();
}

int Route::setStatus(const esc::Net::IPv4Addr &ip,esc::Net::Status status) {
	std::lock_guard<std::mutex> guard(_mutex);
	for(auto it = _table.begin(); it != _table.end(); ++it) {
		if((*it)->dest == ip) {
			if(status == esc::Net::DOWN)
				(*it)->flags &= ~esc::Net::FL_UP;
			else
				(*it)->flags |= esc::Net::FL_UP;
			return 0;
		}
	}
	return -ENOTFOUND;
}

int Route::remove(const esc::Net::IPv4Addr &ip) {
	std::lock_guard<std::mutex> guard(_mutex);
	for(auto it = _table.begin(); it != _table.end(); ++it) {
		if((*it)->dest == ip) {
			_table.erase(it);
			return 0;
		}
	}
	return -ENOTFOUND;
}

void Route::removeAll(const std::shared_ptr<Link> l) {
	std::lock_guard<std::mutex> guard(_mutex);
	for(auto it = _table.begin(); it != _table.end(); ) {
		if((*it)->link == l)
			_table.erase(it);
		else
			++it;
	}
}

void Route::print(esc::OStream &os) {
	std::lock_guard<std::mutex> guard(_mutex);
	for(auto it = _table.begin(); it != _table.end(); ++it) {
		os << (*it)->dest << " " << (*it)->gateway << " " << (*it)->netmask << " ";
		os << (*it)->flags << " " << (*it)->link->name() << "\n";
	}
}
