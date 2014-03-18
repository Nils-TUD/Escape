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

#include <ipc/device.h>
#include <ipc/ipcstream.h>
#include <esc/mem.h>

namespace ipc {

class Client {
public:
	explicit Client() : _shm() {
	}
	virtual ~Client() {
		munmap(_shm);
	}

	char *shm() {
		return _shm;
	}
	void shm(char *ptr) {
		_shm = ptr;
	}

private:
	char *_shm;
};

template<class C = Client>
class ClientDevice : public Device {
	typedef std::map<int,C*> map_type;

public:
	explicit ClientDevice(const char *name,mode_t mode,uint type,uint ops)
		: Device(name,mode,type,ops | DEV_OPEN), _clients(), _sem() {
		set(MSG_DEV_OPEN,std::make_memfun(this,&ClientDevice::open));
		if(ops & DEV_SHFILE)
			set(MSG_DEV_SHFILE,std::make_memfun(this,&ClientDevice::shfile));
		set(MSG_DEV_CLOSE,std::make_memfun(this,&ClientDevice::close));
		if(usemcrt(&_sem,1) < 0)
			throw IPCException("Unable to create user-semaphore");
	}
	virtual ~ClientDevice() {
		usemdestr(&_sem);
	}

	C *get(int fd) {
		typename map_type::iterator it = _clients.find(fd);
		assert(it != _clients.end());
		return it->second;
	}
	const C *get(int fd) const {
		typename map_type::const_iterator it = _clients.find(fd);
		assert(it != _clients.end());
		return it->second;
	}

	void broadcast(msgid_t mid,IPCBuf &ib) {
		usemdown(&_sem);
		for(auto it = _clients.begin(); it != _clients.end(); ++it)
			send(it->first,mid,ib.buffer(),ib.pos());
		usemup(&_sem);
	}

	void add(int fd,C *c) {
		usemdown(&_sem);
		_clients[fd] = c;
		usemup(&_sem);
	}
	void remove(int fd) {
		C *c = get(fd);
		usemdown(&_sem);
		_clients.erase(fd);
		delete c;
		usemup(&_sem);
	}

protected:
	void open(IPCStream &is) {
		add(is.fd(),new C);

		is << 0 << Send(MSG_DEV_OPEN_RESP);
	}

	void shfile(IPCStream &is) {
		CStringBuf<MAX_PATH_LEN> path;
		size_t size;

		C *c = get(is.fd());
		is >> size >> path;
		assert(c->shm() == NULL && !is.error());

		c->shm(static_cast<char*>(joinbuf(path.str(),size,0)));

		is << (c->shm() != NULL ? 0 : -errno) << Send(MSG_DEV_SHFILE_RESP);
	}

	void close(IPCStream &is) {
		remove(is.fd());
		Device::close(is);
	}

private:
	map_type _clients;
	tUserSem _sem;
};

}
