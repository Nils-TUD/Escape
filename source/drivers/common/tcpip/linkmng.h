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
#include <errno.h>
#include <vector>

#include "link.h"

class LinkMng {
	LinkMng() = delete;

public:
	static int add(const std::string &name,const char *path) {
		if(getByName(name))
			return -EEXIST;
		_links.push_back(new Link(name,path));
		return 0;
	}

	static Link *getByName(const std::string &name) {
		for(auto it = _links.begin(); it != _links.end(); ++it) {
			if(name == (*it)->name())
				return *it;
		}
		return NULL;
	}

	static Link *getByIp(const ipc::Net::IPv4Addr &ip) {
		for(auto it = _links.begin(); it != _links.end(); ++it) {
			if(ip == (*it)->ip())
				return *it;
		}
		return NULL;
	}

	static int rem(const std::string &name) {
		for(auto it = _links.begin(); it != _links.end(); ++it) {
			if(name == (*it)->name()) {
				(*it)->status(ipc::Net::KILLED);
				_links.erase(it);
				return 0;
			}
		}
		return -ENOENT;
	}

	static void print(std::ostream &os) {
		for(auto it = _links.begin(); it != _links.end(); ++it) {
			Link *l = *it;
			os << l->name() << " " << l->status() << " ";
			os << l->mac() << " " << l->ip() << " " << l->subnetMask() << " ";
			os << l->rxpackets() << " " << l->txpackets() << " ";
			os << l->rxbytes() << " " << l->txbytes() << "\n";
		}
	}

private:
	static std::vector<Link*> _links;
};
