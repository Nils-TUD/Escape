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
#include <mutex>
#include <memory>

namespace ipc {

class SharedMemory {
public:
	explicit SharedMemory() : addr(), ino(), dev() {
	}
	explicit SharedMemory(char *_addr,inode_t _ino,dev_t _dev)
		: addr(_addr), ino(_ino), dev(_dev) {
	}
	~SharedMemory() {
		if(addr)
			munmap(addr);
	}

	char *addr;
	inode_t ino;
	dev_t dev;
};

template<class C>
class ClientDevice;

/**
 * Default client for ClientDevice.
 */
class Client {
	template<class C>
	friend class ClientDevice;

public:
	explicit Client(int f) : _fd(f), _shm() {
	}
	virtual ~Client() {
	}

	int fd() const {
		return _fd;
	}
	char *shm() {
		return _shm ? _shm->addr : NULL;
	}
	std::shared_ptr<SharedMemory> sharedmem() const {
		return _shm;
	}
	void sharedmem(const std::shared_ptr<SharedMemory> &s) {
		_shm = s;
	}

private:
	int _fd;
	std::shared_ptr<SharedMemory> _shm;
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
		: Device(path,mode,type,ops | DEV_OPEN), _clients(), _mutex() {
		set(MSG_FILE_OPEN,std::make_memfun(this,&ClientDevice::open));
		if(ops & DEV_SHFILE)
			set(MSG_DEV_SHFILE,std::make_memfun(this,&ClientDevice::shfile));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&ClientDevice::close),false);
	}
	/**
	 * Cleans up
	 */
	virtual ~ClientDevice() {
	}

	/**
	 * @return the client with given file-descriptor
	 */
	C *operator[](int fd) {
		typename map_type::iterator it = _clients.find(fd);
		assert(it != _clients.end());
		return it->second;
	}
	const C *operator[](int fd) const {
		typename map_type::const_iterator it = _clients.find(fd);
		assert(it != _clients.end());
		return it->second;
	}

	/**
	 * The same as operator[], but throws an exception if the client does not exist.
	 *
	 * @return the client with given file-descriptor
	 * @throws if the client does not exist
	 */
	C *get(int fd) {
		typename map_type::iterator it = _clients.find(fd);
		if(it == _clients.end())
			VTHROWE("No client with id " << fd,-ENOTFOUND);
		return it->second;
	}
	const C *get(int fd) const {
		typename map_type::const_iterator it = _clients.find(fd);
		if(it == _clients.end())
			VTHROWE("No client with id " << fd,-ENOTFOUND);
		return it->second;
	}

	/**
	 * Broadcasts the given message to all clients
	 *
	 * @param mid the message-id
	 * @param ib the IPCBuf to send
	 */
	void broadcast(msgid_t mid,IPCBuf &ib) {
		std::lock_guard<std::mutex> guard(_mutex);
		for(auto it = _clients.begin(); it != _clients.end(); ++it)
			send(it->first,mid,ib.buffer(),ib.pos());
	}

	/**
	 * Adds the given client to the list
	 *
	 * @param fd the file-descriptor
	 * @param c the client
	 */
	void add(int fd,C *c) {
		std::lock_guard<std::mutex> guard(_mutex);
		_clients[fd] = c;
	}
	/**
	 * Removes the given client from the list
	 *
	 * @param fd the file-descriptor
	 * @param del whether to delete the object
	 */
	void remove(int fd,bool del = true) {
		C *c = get(fd);
		std::lock_guard<std::mutex> guard(_mutex);
		_clients.erase(fd);
		if(del)
			delete c;
	}

	/**
	 * Joins client <c> to the given shared memory.
	 *
	 * @param c the client
	 * @param path the file path
	 * @param size the size of the memory
	 * @param flags the flags to use for joinbuf
	 * @return 0 on success
	 */
	int joinshm(C *c,const char *path,size_t size,uint flags = 0) {
		sFileInfo info;
		int res = stat(path,&info);

		if(res == 0) {
			// check whether we already have this file. we can't map it twice
			std::lock_guard<std::mutex> guard(_mutex);
			res = -ENOENT;
			for(auto it = _clients.begin(); it != _clients.end(); ++it) {
				C *oc = it->second;
				if(c != oc && oc->_shm && oc->_shm->ino == info.inodeNo && oc->_shm->dev == info.device) {
					c->_shm = oc->_shm;
					res = 0;
					break;
				}
			}

			// if not, map it
			if(res == -ENOENT) {
				void *addr = joinbuf(path,size,flags);
				if(!addr)
					res = errno;
				else {
					c->_shm.reset(new SharedMemory(reinterpret_cast<char*>(addr),info.inodeNo,info.device));
					res = 0;
				}
			}
		}
		return res;
	}

protected:
	void open(IPCStream &is) {
		add(is.fd(),new C(is.fd()));

		is << FileOpen::Response(0) << Reply();
	}

	void shfile(IPCStream &is) {
		C *c = get(is.fd());
		char path[MAX_PATH_LEN];
		FileShFile::Request r(path,sizeof(path));
		is >> r;

		assert(c->shm() == NULL && !is.error());
		int res = joinshm(c,path,r.size,0);
		is << FileShFile::Response(res) << Reply();
	}

	void close(IPCStream &is) {
		remove(is.fd());
		Device::close(is);
	}

private:
	map_type _clients;
	std::mutex _mutex;
};

}
