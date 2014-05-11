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
#include <functor.h>
#include <vector>

namespace ipc {

struct Request {
	explicit Request() : fd(), mid(), data(), count(), offset() {
	}
	explicit Request(int _fd,msgid_t _mid,char *_data,size_t _count,size_t _offset = 0)
		: fd(_fd), mid(_mid), data(_data), count(_count), offset(_offset) {
	}

	int fd;
	msgid_t mid;
	char *data;
	size_t count;
	size_t offset;
};

class RequestQueue {
public:
	typedef std::vector<Request>::iterator iterator;
	typedef std::Functor<bool,int,msgid_t,char*,size_t> handler_type;

	explicit RequestQueue(handler_type *handler) : _requests(), _handler(handler) {
	}

	iterator begin() {
		return _requests.begin();
	}
	iterator end() {
		return _requests.end();
	}

	size_t size() const {
		return _requests.size();
	}

	void enqueue(const Request &r) {
		_requests.push_back(r);
	}

	void handle() {
		while(_requests.size() > 0) {
			const Request &req = _requests.front();
			if(!(*_handler)(req.fd,req.mid,req.data,req.count))
				break;
			_requests.erase(begin());
		}
	}

	int cancel(msgid_t mid) {
		int res = 1;
		for(auto it = _requests.begin(); it != _requests.end(); ++it) {
			if(it->mid == mid) {
				_requests.erase(it);
				res = 0;
				break;
			}
		}
		return res;
	}

private:
	std::vector<Request> _requests;
	handler_type *_handler;
};

}
