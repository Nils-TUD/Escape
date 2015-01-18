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
#include <algorithm>
#include <errno.h>
#include <vector>

class Socket;

class RawSocketList {
public:
	typedef std::vector<Socket*> list_type;
	typedef list_type::iterator iterator;

	iterator begin() {
		return _socks.begin();
	}
	iterator end() {
		return _socks.end();
	}

	ssize_t add(Socket *sock) {
		if(contains(sock))
			return -EADDRINUSE;
		_socks.push_back(sock);
		return 0;
	}
	bool contains(Socket *sock) {
		list_type::iterator it;
		it = std::find_if(_socks.begin(),_socks.end(),[sock] (Socket *s) {
			return s == sock;
		});
		return it != _socks.end();
	}
	void remove(Socket *sock) {
		_socks.erase_first(sock);
	}

private:
	list_type _socks;
};
