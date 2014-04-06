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
#include <ipc/proto/file.h>
#include <esc/mem.h>
#include <esc/sync.h>
#include <vthrow.h>

namespace ipc {

/**
 * Default client for ClientDevice.
 */
class Client {
public:
	explicit Client(int fd) : _fd(fd), _shm() {
	}
	virtual ~Client() {
		munmap(_shm);
	}

	int fd() const {
		return _fd;
	}
	char *shm() {
		return _shm;
	}
	void shm(char *ptr) {
		_shm = ptr;
	}

private:
	int _fd;
	char *_shm;
};

/**
 * A device that maintains a list of clients.
 */
template<class C = Client>
class ClientDevice : public Device {
	typedef std::map<int,C*> map_type;
	typedef typename map_type::iterator iterator;

public:
	/**
	 * Creates the device at given path.
	 *
	 * @param path the path
	 * @param mode the permissions to set
	 * @param type the device type
	 * @param ops the supported operations (DEV_*)
	 * @throws if the operation failed
	 */
	explicit ClientDevice(const char *path,mode_t mode,uint type,uint ops)
		: Device(path,mode,type,ops | DEV_OPEN), _clients(), _sem() {
		set(MSG_FILE_OPEN,std::make_memfun(this,&ClientDevice::open));
		if(ops & DEV_SHFILE)
			set(MSG_FILE_SHFILE,std::make_memfun(this,&ClientDevice::shfile));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&ClientDevice::close),false);
		int res;
		if((res = usemcrt(&_sem,1)) < 0)
			VTHROWE("usemcrt",res);
	}
	/**
	 * Cleans up
	 */
	virtual ~ClientDevice() {
		usemdestr(&_sem);
	}

	/**
	 * @return the client with given file-descriptor
	 */
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

	/**
	 * Broadcasts the given message to all clients
	 *
	 * @param mid the message-id
	 * @param ib the IPCBuf to send
	 */
	void broadcast(msgid_t mid,IPCBuf &ib) {
		usemdown(&_sem);
		for(auto it = _clients.begin(); it != _clients.end(); ++it)
			send(it->first,mid,ib.buffer(),ib.pos());
		usemup(&_sem);
	}

	/**
	 * Adds the given client to the list
	 *
	 * @param fd the file-descriptor
	 * @param c the client
	 */
	void add(int fd,C *c) {
		usemdown(&_sem);
		_clients[fd] = c;
		usemup(&_sem);
	}
	/**
	 * Removes the given client from the list
	 *
	 * @param fd the file-descriptor
	 */
	void remove(int fd) {
		C *c = get(fd);
		usemdown(&_sem);
		_clients.erase(fd);
		delete c;
		usemup(&_sem);
	}

protected:
	void open(IPCStream &is) {
		add(is.fd(),new C(is.fd()));

		is << FileOpen::Response(0) << Send(FileOpen::Response::MID);
	}

	void shfile(IPCStream &is) {
		C *c = get(is.fd());
		char path[MAX_PATH_LEN];
		FileShFile::Request r(path,sizeof(path));
		is >> r;
		assert(c->shm() == NULL && !is.error());

		c->shm(static_cast<char*>(joinbuf(r.path.str(),r.size,0)));

		is << FileShFile::Response(c->shm() != NULL ? 0 : -errno) << Send(FileShFile::Response::MID);
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
