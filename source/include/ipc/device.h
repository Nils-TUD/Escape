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
#include <ipc/ipcstream.h>
#include <functor.h>
#include <sstream>
#include <map>

namespace ipc {

class Device {

public:
	typedef std::Functor<void,IPCStream&> handler_type;
	struct Handler {
		explicit Handler() : func(), reply() {
		}
		explicit Handler(handler_type *f,bool r) : func(f), reply(r) {
		}

		handler_type *func;
		bool reply;
	};
	typedef std::map<msgid_t,Handler> oplist_type;

	explicit Device(const char *name,mode_t mode,uint type,uint ops);
	virtual ~Device();
	Device(const Device &) = delete;
	Device &operator=(const Device &) = delete;

	int id() const {
		return _id;
	}
	bool isStopped() const {
		return !_run;
	}
	void stop() {
		_run = false;
	}

	void set(msgid_t op,handler_type *handler,bool reply = true);
	void unset(msgid_t op);
	void loop();
	void handleMsg(msgid_t mid,IPCStream &is);

protected:
	void reply(IPCStream &is,int errcode);
	void close(IPCStream &is) {
		::close(is.fd());
	}

private:
	oplist_type _ops;
	int _id;
	volatile bool _run;
};

}
