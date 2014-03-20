/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/driver.h>
#include <ipc/exception.h>
#include <ipc/ipcstream.h>
#include <functor.h>
#include <sstream>
#include <map>

namespace ipc {

class Device {
public:
	typedef std::Functor<void,IPCStream&> handler_type;
	typedef std::map<msgid_t,handler_type*> oplist_type;

	explicit Device(const char *name,mode_t mode,uint type,uint ops)
		: _ops(), _id(createdev(name,mode,type,ops | DEV_CLOSE)), _run(true) {
		if(_id < 0)
			throw IPCException("createdev failed");
		set(MSG_DEV_CLOSE,std::make_memfun(this,&Device::close));
	}

	virtual ~Device() {
		for(auto it = _ops.begin(); it != _ops.end(); ) {
			auto old = it++;
			delete &*old;
		}
		::close(_id);
	}

	Device(const Device &) = delete;
	Device &operator=(const Device &) = delete;

	int id() const {
		return _id;
	}

	void set(msgid_t op,handler_type *handler) {
		oplist_type::iterator it = _ops.find(op);
		if(it != _ops.end())
			delete it->second;
		_ops[op] = handler;
	}

	void loop() {
		char buf[IPC_DEF_SIZE];
		while(_run) {
			msgid_t mid;
			int fd = getwork(_id,&mid,buf,sizeof(buf),0);
			if(EXPECT_FALSE(fd < 0)) {
				if(fd != -EINTR)
					throw IPCException("getwork failed");
				continue;
			}

			IPCStream is(fd,buf,sizeof(buf));
			handleMsg(mid,is);
		}
	}

	bool isStopped() const {
		return !_run;
	}
	void stop() {
		_run = false;
	}

protected:
	void handleMsg(msgid_t mid,IPCStream &is) {
		oplist_type::iterator it = _ops.find(mid);
		try {
			if(EXPECT_FALSE(it == _ops.end()))
				is << -ENOTSUP << Send(MSG_DEF_RESPONSE);
			else
				(*(*it).second)(is);
		}
		catch(const IPCException &e) {
			printe("Client %d: handling message %d failed: %s",is.fd(),mid,e.what());
		}
	}

	void close(IPCStream &is) {
		::close(is.fd());
	}

private:
	oplist_type _ops;
	int _id;
	volatile bool _run;
};

}
