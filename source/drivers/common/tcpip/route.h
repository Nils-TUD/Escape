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

#include "ipv4addr.h"
#include "nic.h"

class Route {
public:
	enum {
		FL_USE_GW	= 0x1,
		FL_UP		= 0x2,
	};

	explicit Route(const IPv4Addr &dst,const IPv4Addr &nm,const IPv4Addr &gw,uint fl,NICDevice *n)
		: dest(dst), netmask(nm), gateway(gw), flags(fl), nic(n) {
	}

	static const Route *find(const IPv4Addr &ip);
	static int insert(const IPv4Addr &ip,const IPv4Addr &nm,const IPv4Addr &gw,uint flags,NICDevice *nic);

	IPv4Addr dest;
	IPv4Addr netmask;
	IPv4Addr gateway;
	uint flags;
	NICDevice *nic;

private:
	static std::vector<Route*> _table;
};
