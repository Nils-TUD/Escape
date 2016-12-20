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

#include <esc/ipc/device.h>
#include <esc/ipc/ipcstream.h>
#include <esc/proto/file.h>
#include <esc/proto/device.h>
#include <esc/vthrow.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sync.h>
#include <limits.h>
#include <memory>
#include <mutex>

namespace esc {

class SharedMemory {
public:
	explicit SharedMemory() : addr(), size(), ino(), dev() {
	}
	explicit SharedMemory(int _fd,char *_addr,size_t _size,ino_t _ino,dev_t _dev)
		: fd(_fd), addr(_addr), size(_size), ino(_ino), dev(_dev) {
	}
	~SharedMemory() {
		if(addr)
			munmap(addr);
		close(fd);
	}

	int fd;
	char *addr;
	size_t size;
	ino_t ino;
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
		if(ops & DEV_DELEGATE)
			set(MSG_DEV_DELEGATE,std::make_memfun(this,&ClientDevice::delegate));
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
		return it != _clients.end() ? it->second : NULL;
	}
	const C *operator[](int fd) const {
		return const_cast<ClientDevice*>(this)[fd];
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
		return const_cast<ClientDevice*>(this)->get(fd);
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
	 * @param fd the file
	 * @param flags the flags to use for joinbuf
	 * @return 0 on success
	 */
	int joinshm(C *c,int fd,uint flags = 0) {
		struct stat info;
		int res = fstat(fd,&info);

		if(res == 0) {
			// check whether we already have this file. we can't map it twice
			std::lock_guard<std::mutex> guard(_mutex);
			res = -ENOENT;
			for(auto it = _clients.begin(); it != _clients.end(); ++it) {
				C *oc = it->second;
				if(c != oc && oc->_shm && oc->_shm->ino == info.st_ino && oc->_shm->dev == info.st_dev) {
					c->_shm = oc->_shm;
					res = 0;
					break;
				}
			}

			// if not, map it
			if(res == -ENOENT) {
				void *addr = mmap(NULL,info.st_size,0,PROT_READ | PROT_WRITE,MAP_SHARED | flags,fd,0);
				if(!addr)
					res = -errno;
				else {
					c->_shm.reset(new SharedMemory(
						fd,reinterpret_cast<char*>(addr),info.st_size,info.st_ino,info.st_dev));
					res = 0;
				}
			}
		}
		return res;
	}

protected:
	void open(IPCStream &is) {
		add(is.fd(),new C(is.fd()));

		is << FileOpen::Response::success(0) << Reply();
	}

	void delegate(IPCStream &is) {
		C *c = get(is.fd());
		DevDelegate::Request r;
		is >> r;

		int res = -EINVAL;
		if(r.arg == DEL_ARG_SHFILE) {
			assert(c->shm() == NULL && !is.error());
			res = joinshm(c,r.nfd,0);
		}
		is << DevDelegate::Response(res) << Reply();
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
