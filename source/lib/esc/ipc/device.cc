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

#include <esc/ipc/device.h>
#include <esc/vthrow.h>
#include <sys/common.h>
#include <sys/messages.h>

namespace esc {

Device::Device(const char *path,mode_t mode,uint type,uint ops)
	: _ops(), _id(createdev(path,mode,type,ops | DEV_CLOSE)), _run(true) {
	if(_id < 0)
		VTHROWE("createdev(" << path << ")",_id);
	set(MSG_FILE_CLOSE,std::make_memfun(this,&Device::close),false);
}

Device::~Device() {
	for(auto it = _ops.begin(); it != _ops.end(); ) {
		auto old = it++;
		delete old->second.func;
	}
	::close(_id);
}

void Device::set(msgid_t op,handler_type *handler,bool rep) {
	assert(handler != NULL);
	unset(op);
	_ops[op] = Handler(handler,rep);
}

void Device::unset(msgid_t op) {
	oplist_type::iterator it = _ops.find(op);
	if(it != _ops.end()) {
		delete it->second.func;
		_ops.erase(it);
	}
}

void Device::loop() {
	ulong buf[IPC_DEF_SIZE / sizeof(ulong)];
	while(_run) {
		msgid_t mid;
		int fd = getwork(_id,&mid,buf,sizeof(buf),0);
		if(EXPECT_FALSE(fd < 0)) {
			/* just log that it failed. maybe a client has sent a message that was too big */
			if(fd != -EINTR)
				printe("getwork failed");
			continue;
		}

		IPCStream is(fd,buf,sizeof(buf),mid);
		handleMsg(mid,is);
	}
}

void Device::reply(IPCStream &is,errcode_t errcode) {
	try {
		is << errcode << Reply();
	}
	catch(...) {
		printe("Client %d: sending error-code failed",is.fd());
	}
}

void Device::handleMsg(msgid_t mid,IPCStream &is) {
	oplist_type::iterator it = _ops.find(mid & 0xFFFF);
	Handler &h = (*it).second;
	try {
		if(EXPECT_FALSE(it == _ops.end()))
			reply(is,-ENOTSUP);
		else
			(*h.func)(is);
	}
	catch(const esc::default_error &e) {
		// TODO printe is annoying here since it prints errno, which is typically nonsense.
		printe("Client %d, message %u: %s",is.fd(),mid & 0xFFFF,e.what());
		if(h.reply)
			reply(is,e.error() ? e.error() : -EINVAL);
	}
	catch(...) {
		printe("Client %d, message %u: unknown error",is.fd(),mid & 0xFFFF);
		if(h.reply)
			reply(is,-EINVAL);
	}
}

}
