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

#pragma once

#include <esc/common.h>
#include <vector>

#include "common.h"
#include "link.h"

class Route {
public:
	explicit Route(const ipc::Net::IPv4Addr &dst,const ipc::Net::IPv4Addr &nm,
			const ipc::Net::IPv4Addr &gw,uint fl,const std::shared_ptr<Link> &l)
		: dest(dst), netmask(nm), gateway(gw), flags(fl), link(l) {
	}

	static int insert(const ipc::Net::IPv4Addr &ip,const ipc::Net::IPv4Addr &nm,
		const ipc::Net::IPv4Addr &gw,uint flags,const std::shared_ptr<Link> &l);
	static const Route *find(const ipc::Net::IPv4Addr &ip);
	static int setStatus(const ipc::Net::IPv4Addr &ip,ipc::Net::Status status);
	static int remove(const ipc::Net::IPv4Addr &ip);
	static void removeAll(const std::shared_ptr<Link> &link);
	static void print(std::ostream &os);

	ipc::Net::IPv4Addr dest;
	ipc::Net::IPv4Addr netmask;
	ipc::Net::IPv4Addr gateway;
	uint flags;
	std::shared_ptr<Link> link;

private:
	static std::vector<Route*> _table;
};
