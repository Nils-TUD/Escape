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

#include <esc/common.h>
#include <esc/thread.h>
#include <ipc/proto/socket.h>
#include <ipc/clientdevice.h>
#include <sstream>
#include <fstream>
#include <dns.h>
#include <signal.h>
#include <stdlib.h>

using namespace ipc;

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
		  sock("/dev/socket",Socket::SOCK_STREAM,Socket::PROTO_TCP) {
		std::istringstream isurl(url);
		isurl.getline(domain,'/');
		isurl.getline(path,'#');
	}

	bool redirected;
	std::string domain;
	std::string path;
	State state;
	size_t contentlen;
	Socket sock;
};

class ScopedBuf {
public:
	explicit ScopedBuf(char *d,bool free) : _data(d), _free(free) {
	}
	~ScopedBuf() {
		if(_free)
			delete[] _data;
	}

	char *data() {
		return _data;
	}

private:
	char *_data;
	bool _free;
};

class HTTPDevice : public ClientDevice<HTTPClient> {
public:
	explicit HTTPDevice(const char *name,mode_t mode)
		: ClientDevice(name,mode,DEV_TYPE_CHAR,
			DEV_OPEN | DEV_CANCELSIG | DEV_SHFILE | DEV_READ | DEV_CLOSE) {
		set(MSG_FILE_OPEN,std::make_memfun(this,&HTTPDevice::open));
		set(MSG_FILE_READ,std::make_memfun(this,&HTTPDevice::read));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&HTTPDevice::close),false);
	}

	void open(IPCStream &is) {
		char urlbuf[MAX_PATH_LEN];
		FileOpen::Request r(urlbuf,sizeof(urlbuf));
		is >> r;

		int res = startthread(handlerThread,(void*)is.fd());
		if(res >= 0) {
			add(is.fd(),new HTTPClient(is.fd(),urlbuf));
			::bindto(is.fd(),res);
		}
		is << FileOpen::Response(res >= 0 ? 0 : res) << Reply();
	}

	void read(IPCStream &is) {
		HTTPClient *c = (*this)[is.fd()];
		FileRead::Request r;
		is >> r;

		// take care that the buffer is deleted if an exception throws
		ScopedBuf buf(r.shmemoff != -1 ? c->shm() + r.shmemoff : new char[r.count],r.shmemoff == -1);

		if(c->state == STATE_OPEN) {
			Socket::Addr addr;
			addr.family = Socket::AF_INET;
			addr.d.ipv4.addr = std::DNS::getHost(c->domain.c_str()).value();
			addr.d.ipv4.port = 80;
			c->sock.connect(addr);
			c->state = STATE_CONNECTED;
		}

		if(c->state == STATE_CONNECTED) {
			std::ostringstream req;
			req << "GET " << c->path << " HTTP/1.0\n";
			req << "Host: " << c->domain << "\n\n";

			c->sock.send(req.str().c_str(),req.str().length());
			c->state = STATE_REQSENT;
		}

		if(c->state == STATE_REQSENT) {
			std::ifstream resp;
			// TODO we should make sure, that the stream uses the same buffer-size as the client
			// to ensure that we can always return all of the remaining data in the buffer
			resp.open(c->sock.fd(),std::ios_base::in | std::ios_base::signals);
			while(resp.good()) {
				std::string line;
				resp.getline(line,'\n');

				// maybe we received a signal?
				if(resp.eof()) {
					is << FileRead::Response(0) << Reply();
					return;
				}

				// redirection?
				if(line.find("Location: ") == 0) {
					// cut off the \r
					const size_t loclen = SSTRLEN("Location: ");
					std::string newloc = line.substr(loclen,line.length() - loclen - 1);

					// check if it's valid (we just ignore multiple redirections here)
					print("Redirected to %s",newloc.c_str());
					if(c->redirected) {
						is << FileRead::Response(-EINVAL) << Reply();
						return;
					}
					// only http is supported
					if(strncmp(newloc.c_str(),"http://",7) != 0) {
						is << FileRead::Response(-ENOTSUP) << Reply();
						return;
					}

					// destroy current socket and create a new one (ensure that we keep the shm)
					std::shared_ptr<SharedMemory> shm = c->sharedmem();
					delete c;
					HTTPClient *nc = new HTTPClient(is.fd(),newloc.c_str() + SSTRLEN("http://"),true);
					nc->sharedmem(shm);

					// just assign the new client to the same slot
					add(is.fd(),nc);
					// reset the stream to read the request again
					is.reset();
					read(is);
					return;
				}

				if(line.find("Content-Length: ") == 0)
					c->contentlen = strtoul(line.c_str() + SSTRLEN("Content-Length: "),NULL,10);
				// we grabbed the \r
				else if(line.length() == 1)
					break;
			}
			c->state = STATE_RESP;

			// if there is something left in the buffer, reply that first
			size_t left = resp.rdbuf()->remaining();
			if(left > 0) {
				if(r.shmemoff != -1)
					memcpy(buf.data(),resp.rdbuf()->input(),left);
				is << FileRead::Response(left) << Reply();
				if(r.shmemoff == -1)
					is << ReplyData(resp.rdbuf()->input(),left);
				c->contentlen -= left;
				return;
			}
		}

		if(c->state == STATE_RESP) {
			size_t max = std::min(c->contentlen,r.count);
			size_t count = max;
			if(count > 0)
				count = c->sock.receive(buf.data(),max);
			is << FileRead::Response(count) << Reply();
			if(r.shmemoff == -1) {
				if(count)
					is << ReplyData(buf.data(),count);
			}
			c->contentlen -= count;
		}
	}

	void close(IPCStream &is) {
		ClientDevice::close(is);
		// only the started threads receive the close-message. terminate the thread in this case
		exit(0);
	}
};

static HTTPDevice *dev;

static int handlerThread(void *arg) {
	::bindto(reinterpret_cast<int>(arg),gettid());
	dev->loop();
	return 0;
}

static void sigcancel(int) {
	signal(SIG_CANCEL,sigcancel);
}

int main() {
	if(signal(SIG_CANCEL,sigcancel) == SIG_ERR)
		error("Unable to register signal-handler for SIG_CANCEL");

	dev = new HTTPDevice("/dev/http",0444);
	dev->loop();
	return 0;
}
