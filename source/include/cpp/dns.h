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
#include <esc/proto/net.h>

namespace std {

/**
 * The DNS class allows you to resolve names to IP addresses.
 */
class DNS {
	DNS() = delete;

public:
	/**
	 * @return the file in which the nameserver is stored
	 */
	static const char *getResolveFile() {
		return "/sys/net/nameserver";
	}

	/**
	 * Gets the host for given name. It might also be an IP address, in which case it is not
	 * resolved, but only translated in an ipc::Net::IPv4Addr object.
	 *
	 * @param name the hostname
	 * @param timeout the timeout to use in ms
	 * @return the IP address
	 * @throws if the operation failed
	 */
	static ipc::Net::IPv4Addr getHost(const char *name,uint timeout = 1000);

	/**
	 * Checks whether the given hostname is an IP address.
	 *
	 * @param name the hostname
	 * @return true if so
	 */
	static bool isIPAddress(const char *name);

	/**
	 * Resolves the given name into an IP address.
	 *
	 * @param name the domain name
	 * @param timeout the timeout to use in ms
	 * @return the ip address
	 * @throws if the operation failed
	 */
	static ipc::Net::IPv4Addr resolve(const char *name,uint timeout = 1000);

private:
	static void sigalarm(int) {
	}
	static void convertHostname(char *dst,const char *src,size_t len);
	static size_t questionLength(const uint8_t *data);

	static uint16_t _nextId;
	static ipc::Net::IPv4Addr _nameserver;
};

}
