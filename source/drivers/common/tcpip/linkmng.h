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

#include <sys/common.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <errno.h>
#include <signal.h>
#include <memory>
#include <vector>

#include "link.h"
#include "route.h"

class LinkMng {
	LinkMng() = delete;

public:
	static int add(const std::string &name,const char *path) {
		std::lock_guard<std::mutex> guard(_mutex);
		if(doGetByName(name))
			return -EEXIST;
		_links.push_back(std::shared_ptr<Link>(new Link(name,path)));
		return 0;
	}

	static std::shared_ptr<Link> getByName(const std::string &name) {
		std::lock_guard<std::mutex> guard(_mutex);
		return doGetByName(name);
	}

	static std::shared_ptr<Link> getByIp(const esc::Net::IPv4Addr &ip) {
		std::lock_guard<std::mutex> guard(_mutex);
		for(auto it = _links.begin(); it != _links.end(); ++it) {
			if(ip == (*it)->ip())
				return *it;
		}
		return std::shared_ptr<Link>();
	}

	static int rem(const std::string &name) {
		std::lock_guard<std::mutex> guard(_mutex);
		for(auto it = _links.begin(); it != _links.end(); ++it) {
			if(name == (*it)->name()) {
				(*it)->status(esc::Net::KILLED);
				kill(getpid(),SIGUSR1);
				Route::removeAll(*it);
				_links.erase(it);
				return 0;
			}
		}
		return -ENOTFOUND;
	}

	static void print(esc::OStream &os) {
		std::lock_guard<std::mutex> guard(_mutex);
		for(auto it = _links.begin(); it != _links.end(); ++it) {
			const std::shared_ptr<Link> &l = *it;
			os << l->name() << " " << l->status() << " ";
			os << l->mac() << " " << l->ip() << " " << l->subnetMask() << " ";
			os << l->mtu() << " ";
			os << l->rxpackets() << " " << l->txpackets() << " ";
			os << l->rxbytes() << " " << l->txbytes() << "\n";
		}
	}

private:
	static std::shared_ptr<Link> doGetByName(const std::string &name) {
		for(auto it = _links.begin(); it != _links.end(); ++it) {
			if(name == (*it)->name())
				return *it;
		}
		return std::shared_ptr<Link>();
	}

	static std::mutex _mutex;
	static std::vector<std::shared_ptr<Link>> _links;
};
