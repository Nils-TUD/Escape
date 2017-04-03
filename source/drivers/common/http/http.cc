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

#include <esc/ipc/clientdevice.h>
#include <esc/proto/socket.h>
#include <esc/stream/istringstream.h>
#include <esc/stream/fstream.h>
#include <esc/stream/std.h>
#include <esc/dns.h>
#include <sys/common.h>
#include <sys/thread.h>
#include <signal.h>
#include <stdlib.h>

using namespace esc;

static int handlerThread(void *);

enum State {
	STATE_OPEN,
	STATE_CONNECTED,
	STATE_REQSENT,
	STATE_RESP
};

class HTTPClient : public Client {
public:
	// default-value because the ClientDevice-template calls HTTPClient(f); but that code isn't used
	explicit HTTPClient(int f,const char *url = "",bool _redirected = false)
		: Client(f), redirected(_redirected), domain(), path("/"), state(STATE_OPEN), contentlen(),
		  remaining(), header(), sock(Socket::SOCK_STREAM,Socket::PROTO_TCP) {
		esc::IStringStream isurl(url);
		isurl.getline(domain,'/');
		isurl.getline(path,'#');
		if(path.empty())
			path = "/";
	}
	~HTTPClient() {
		delete header;
	}

	bool redirected;
	std::string domain;
	std::string path;
	State state;
	size_t contentlen;
	size_t remaining;
	esc::FStream *header;
	Socket sock;
};

class HTTPDevice : public ClientDevice<HTTPClient> {
public:
	explicit HTTPDevice(const char *name,mode_t mode)
		: ClientDevice(name,mode,DEV_TYPE_CHAR,
			DEV_OPEN | DEV_CANCELSIG | DEV_SIZE | DEV_DELEGATE | DEV_READ | DEV_CLOSE) {
		set(MSG_FILE_OPEN,std::make_memfun(this,&HTTPDevice::open));
		set(MSG_FILE_SIZE,std::make_memfun(this,&HTTPDevice::filesize));
		set(MSG_FILE_READ,std::make_memfun(this,&HTTPDevice::read));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&HTTPDevice::close),false);
	}

	void open(IPCStream &is) {
		char urlbuf[MAX_PATH_LEN];
		FileOpen::Request r(urlbuf,sizeof(urlbuf));
		is >> r;

		int res = startthread(handlerThread,new int(is.fd()));
		if(res >= 0) {
			add(is.fd(),new HTTPClient(is.fd(),urlbuf));
			// we don't know whether the thread starts before we're finished or not. so, bind it
			// here to prevent that we get a request for this channel and bind it in the thread-
			// function to ensure that the thread gets already arrived messages
			::bindto(is.fd(),res);
			is << FileOpen::Response::success(res) << Reply();
		}
		else
			is << FileOpen::Response::error(res) << Reply();
	}

	void filesize(IPCStream &is) {
		HTTPClient *c = (*this)[is.fd()];
		ssize_t res = 1;
		if(c->state != STATE_RESP)
			res = readHeader(c,is.fd());
		if(res == 1)
			res = c->contentlen;
		is << FileSize::Response::result(res) << Reply();
	}

	void read(IPCStream &is) {
		HTTPClient *c = (*this)[is.fd()];
		FileRead::Request r;
		is >> r;

		// take care that the buffer is deleted if an exception throws
		DataBuf buf(r.count,c->shm(),r.shmemoff);

		// at first, we have to send the request and read the header
		if(c->state != STATE_RESP) {
			int res = readHeader(c,is.fd());
			if(res <= 0) {
				is << FileRead::Response::result(res) << Reply();
				return;
			}
			// refresh client. it might have changed
			c = (*this)[is.fd()];
		}

		// if there is something left in the buffer, reply that first
		if(c->header && c->header->inbuflen() > 0) {
			size_t amount = std::min(r.count,c->header->inbuflen());
			if(r.shmemoff != -1)
				c->header->read(buf.data(),amount);
			is << FileRead::Response::success(amount) << Reply();
			if(r.shmemoff == -1) {
				// not nice, but the easiest way
				char tmp[amount];
				c->header->read(tmp,amount);
				is << ReplyData(tmp,amount);
			}
			c->remaining -= amount;
			return;
		}

		if(c->state == STATE_RESP) {
			size_t max = std::min(c->remaining,r.count);
			size_t count = max;
			if(count > 0)
				count = c->sock.receive(buf.data(),max);
			is << FileRead::Response::success(count) << Reply();
			if(r.shmemoff == -1) {
				if(count)
					is << ReplyData(buf.data(),count);
			}
			c->remaining -= count;
		}
	}

	void close(IPCStream &is) {
		ClientDevice::close(is);
		// only the started threads receive the close-message. terminate the thread in this case
		exit(0);
	}

private:
	ssize_t readHeader(HTTPClient *c,int fd) {
		if(c->state == STATE_OPEN) {
			Socket::Addr addr;
			addr.family = Socket::AF_INET;
			addr.d.ipv4.addr = esc::DNS::getHost(c->domain.c_str()).value();
			addr.d.ipv4.port = 80;
			c->sock.connect(addr);
			c->state = STATE_CONNECTED;
		}

		if(c->state == STATE_CONNECTED) {
			esc::OStringStream req;
			req << "GET " << c->path << " HTTP/1.0\n";
			req << "Host: " << c->domain << "\n\n";

			c->sock.send(req.str().c_str(),req.str().length());
			c->state = STATE_REQSENT;
		}

		if(c->state == STATE_REQSENT) {
			// TODO we should make sure, that the stream uses the same buffer-size as the client
			// to ensure that we can always return all of the remaining data in the buffer
			c->header = new FStream(c->sock.fd(),"rs");
			while(c->header->good()) {
				std::string line;
				c->header->getline(line,'\n');

				// maybe we received a signal?
				if(c->header->bad())
					return 0;

				// redirection?
				if(line.find("Location: ") == 0) {
					// cut off the \r
					const size_t loclen = SSTRLEN("Location: ");
					std::string newloc = line.substr(loclen,line.length() - loclen - 1);

					// check if it's valid (we just ignore multiple redirections here)
					print("Redirected to %s",newloc.c_str());
					if(c->redirected)
						return -EINVAL;
					// only http is supported
					if(strncmp(newloc.c_str(),"http://",7) != 0)
						return -ENOTSUP;

					// destroy current socket and create a new one (ensure that we keep the shm)
					std::shared_ptr<SharedMemory> shm = c->sharedmem();
					delete c;
					HTTPClient *nc = new HTTPClient(fd,newloc.c_str() + SSTRLEN("http://"),true);
					nc->sharedmem(shm);

					// just assign the new client to the same slot
					add(fd,nc);
					return readHeader(nc,fd);
				}

				if(line.find("Content-Length: ") == 0) {
					c->contentlen = strtoul(line.c_str() + SSTRLEN("Content-Length: "),NULL,10);
					c->remaining = c->contentlen;
				}
				// we grabbed the \r
				else if(line.length() == 1)
					break;
			}
			c->state = STATE_RESP;
		}
		return 1;
	}
};

static HTTPDevice *dev;

static int handlerThread(void *arg) {
	int *fd = reinterpret_cast<int*>(arg);
	// see comment above
	::bindto(*fd,gettid());
	dev->loop();
	delete fd;
	return 0;
}

static void sigcancel(int) {
	signal(SIGCANCEL,sigcancel);
}

int main() {
	if(signal(SIGCANCEL,sigcancel) == SIG_ERR)
		error("Unable to register signal-handler for SIGCANCEL");

	dev = new HTTPDevice("/dev/http",0400);
	dev->loop();
	return 0;
}
