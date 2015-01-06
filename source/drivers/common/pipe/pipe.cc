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

#include <sys/common.h>
#include <esc/ipc/requestqueue.h>
#include <esc/ipc/clientdevice.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];

class VarRingBuf {
public:
	explicit VarRingBuf(size_t size) : _data(new uint8_t[size]), _size(size), _rdpos(), _wrpos(), _last() {
	}
	~VarRingBuf() {
		delete[] _data;
	}

	void *push(size_t size) {
		if(_wrpos >= _rdpos) {
			if(_size - _wrpos >= size) {
				_wrpos += size;
				return _data + _wrpos - size;
			}
			else if(_rdpos > size) {
				_last = _wrpos;
				_wrpos = size;
				return _data;
			}
		}
		else if(_rdpos - _wrpos > size) {
			_wrpos += size;
			return _data + _wrpos - size;
		}
		return NULL;
	}

	void *pull(size_t *size) {
		if(_wrpos == _rdpos)
			return NULL;

		if(_rdpos == _last) {
			_rdpos = 0;
			_last = 0;
		}
		if(_wrpos > _rdpos)
			*size = std::min(_wrpos - _rdpos,*size);
		else
			*size = std::min(_size - _rdpos,*size);
		void *res = _data + _rdpos;
		_rdpos += *size;
		return res;
	}

private:
	uint8_t *_data;
	size_t _size;
	size_t _rdpos;
	size_t _wrpos;
	size_t _last;
};

class PipeClient : public esc::Client {
public:
	enum {
		RINGBUF_SIZE	= 128 * 1024
	};
	enum {
		FL_READ			= 1,
		FL_WRITE		= 2
	};

	explicit PipeClient(int f,uint _flags = FL_WRITE)
		: esc::Client(f), partner(), pendingRead(), pendingWrite(), ringbuf(), flags(_flags) {
		if(flags & FL_READ)
			ringbuf = new VarRingBuf(RINGBUF_SIZE);
	}
	virtual ~PipeClient() {
		delete ringbuf;
	}

	void replyRead() {
		if(pendingRead.count == 0)
			return;

		/* check how much we can/want pull out of the ringbuffer */
		size_t size = pendingRead.count;
		void *data = ringbuf->pull(&size);
		if(!data) {
			/* EOF */
			if(partner == NULL) {
				esc::IPCStream is(pendingRead.fd,buffer,sizeof(buffer),pendingRead.mid);
				is << esc::FileRead::Response(0) << esc::Reply();
				pendingRead.count = 0;
			}
			return;
		}

		/* reply that data */
		esc::IPCStream is(pendingRead.fd,buffer,sizeof(buffer),pendingRead.mid);
		if(pendingRead.offset != (size_t)-1)
			memcpy(shm() + pendingRead.offset,data,size);
		is << esc::FileRead::Response(size) << esc::Reply();
		if(pendingRead.offset == (size_t)-1)
			is << esc::ReplyData(data,size);

		/* invalidate pending read */
		pendingRead.count = 0;
		/* check if we can write new data now */
		if(partner)
			partner->replyWrite();
	}

	void replyWrite() {
		if(pendingWrite.count == 0)
			return;

		/* EOF */
		if(!partner) {
			esc::IPCStream is(pendingWrite.fd,buffer,sizeof(buffer),pendingWrite.mid);
			/* skip data message, if not already done */
			if(!pendingWrite.data && pendingWrite.offset == (size_t)-1)
				is >> esc::ReceiveData(NULL,0);
			is << esc::FileWrite::Response(-EDESTROYED) << esc::Reply();
			delete[] pendingWrite.data;
			pendingWrite.count = 0;
		}
		else {
			/* try to get a push-position */
			void *pos = partner->ringbuf->push(pendingWrite.count);
			if(pos) {
				esc::IPCStream is(pendingWrite.fd,buffer,sizeof(buffer),pendingWrite.mid);
				if(pendingWrite.offset == (size_t)-1) {
					/* if we had to allocate the data, write it from there */
					if(pendingWrite.data) {
						memcpy(pos,pendingWrite.data,pendingWrite.count);
						delete[] pendingWrite.data;
					}
					/* otherwise read it directly to that position */
					else
						is >> esc::ReceiveData(pos,pendingWrite.count);
				}
				/* copy from shared memory */
				else
					memcpy(pos,shm() + pendingWrite.offset,pendingWrite.count);

				/* reply and invalidate */
				is << esc::FileWrite::Response(pendingWrite.count) << esc::Reply();
				pendingWrite.count = 0;
				/* check if somebody wanted to read */
				partner->replyRead();
			}
			/* if we don't have space atm, we still have to read the data-message */
			else if(!pendingWrite.data && pendingWrite.offset == (size_t)-1) {
				esc::IPCStream is(pendingWrite.fd,buffer,sizeof(buffer),pendingWrite.mid);
				pendingWrite.data = new char[pendingWrite.count];
				is >> esc::ReceiveData(pendingWrite.data,pendingWrite.count);
			}
		}
	}

	PipeClient *partner;
	esc::Request pendingRead;
	esc::Request pendingWrite;
	VarRingBuf *ringbuf;
	uint flags;
};

class PipeDevice : public esc::ClientDevice<PipeClient> {
public:
	explicit PipeDevice(const char *path,mode_t mode)
		: esc::ClientDevice<PipeClient>(path,mode,DEV_TYPE_CHAR,
			DEV_CREATSIBL | DEV_SHFILE | DEV_CANCEL | DEV_READ | DEV_WRITE | DEV_CLOSE) {
		set(MSG_DEV_CANCEL,std::make_memfun(this,&PipeDevice::cancel));
		set(MSG_DEV_CREATSIBL,std::make_memfun(this,&PipeDevice::creatsibl));
		set(MSG_FILE_READ,std::make_memfun(this,&PipeDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&PipeDevice::write));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&PipeDevice::close));
	}

	void cancel(esc::IPCStream &is) {
		PipeClient *c = (*this)[is.fd()];
		msgid_t mid;
		is >> mid;

		int res = -EINVAL;
		if((mid & 0xFFFF) == MSG_FILE_WRITE) {
			/* we will not necessarily answer a pending read/write immediately. thus, don't tell the
			 * kernel that we do that but just cancel the request in every case */
			if(c->pendingWrite.count > 0 && c->pendingWrite.mid == mid) {
				res = 0;
				c->pendingWrite.count = 0;
			}
		}
		else if((mid & 0xFFFF) == MSG_FILE_READ) {
			if(c->pendingRead.count > 0 && c->pendingRead.mid == mid) {
				res = 0;
				c->pendingRead.count = 0;
			}
		}

		is << res << esc::Reply();
	}

	void creatsibl(esc::IPCStream &is) {
		PipeClient *c = (*this)[is.fd()];
		esc::FileCreatSibl::Request r;
		is >> r;

		int res = 0;
		if(c->flags != PipeClient::FL_WRITE)
			res = -EINVAL;
		else {
			PipeClient *nc = new PipeClient(r.nfd,PipeClient::FL_READ);
			nc->partner = c;
			c->partner = nc;
			add(r.nfd,nc);
		}
		is << esc::FileCreatSibl::Response(res) << esc::Reply();
	}

	void read(esc::IPCStream &is) {
		PipeClient *c = (*this)[is.fd()];
		esc::FileRead::Request r;
		is >> r;

		if(c->pendingRead.count != 0) {
			is << esc::FileRead::Response(-EINVAL) << esc::Reply();
			return;
		}
		if(~c->flags & PipeClient::FL_READ) {
			is << esc::FileRead::Response(-EACCES) << esc::Reply();
			return;
		}

		c->pendingRead.fd = is.fd();
		c->pendingRead.mid = is.msgid();
		c->pendingRead.count = r.count;
		c->pendingRead.offset = r.shmemoff;
		c->replyRead();
	}

	void write(esc::IPCStream &is) {
		PipeClient *c = (*this)[is.fd()];
		esc::FileWrite::Request r;
		is >> r;

		int res = 0;
		if(c->pendingWrite.count != 0 || r.count > PipeClient::RINGBUF_SIZE) {
			res = -EINVAL;
			goto error;
		}
		if(~c->flags & PipeClient::FL_WRITE) {
			res = -EACCES;
			goto error;
		}

		c->pendingWrite.fd = is.fd();
		c->pendingWrite.mid = is.msgid();
		c->pendingWrite.count = r.count;
		c->pendingWrite.offset = r.shmemoff;
		c->pendingWrite.data = NULL;
		c->replyWrite();
		return;

	error:
		/* skip data message */
		if(r.shmemoff == -1)
			is >> esc::ReceiveData(NULL,0);
		is << esc::FileWrite::Response(res) << esc::Reply();
	}

	void close(esc::IPCStream &is) {
		PipeClient *c = (*this)[is.fd()];
		if(c->partner) {
			c->partner->partner = NULL;
			if(c->flags & PipeClient::FL_WRITE)
				c->partner->replyRead();
			else
				c->partner->replyWrite();
		}
		esc::ClientDevice<PipeClient>::close(is);
	}
};

int main() {
	PipeDevice dev("/dev/pipe",0666);
	dev.loop();
	return EXIT_SUCCESS;
}
