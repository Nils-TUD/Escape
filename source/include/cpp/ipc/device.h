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

#include <sys/common.h>
#include <sys/driver.h>
#include <ipc/ipcstream.h>
#include <functor.h>
#include <sstream>
#include <map>

namespace ipc {

/**
 * The base class for all devices
 */
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

	/**
	 * Creates the device at given path
	 *
	 * @param path the path
	 * @param mode the permissions to set
	 * @param type the type of device
	 * @param ops the supported operations (DEV_*)
	 */
	explicit Device(const char *path,mode_t mode,uint type,uint ops);
	/**
	 * Closes the device
	 */
	virtual ~Device();

	/**
	 * No copying/cloning
	 */
	Device(const Device &) = delete;
	Device &operator=(const Device &) = delete;

	/**
	 * @return the file-descriptor for the device
	 */
	int id() const {
		return _id;
	}
	/**
	 * @return true if the device-loop should stop
	 */
	bool isStopped() const {
		return !_run;
	}
	/**
	 * Stops the device-loop.
	 */
	void stop() {
		_run = false;
	}

	/**
	 * Binds this device to the given thread
	 *
	 * @param tid the thread-id
	 */
	void bindto(tid_t tid) {
		::bindto(_id,tid);
	}

	/**
	 * Registers the given handler for <op>.
	 *
	 * @param op the operation (message-id)
	 * @param handler your desired handler
	 * @param reply if the operation has a reply
	 */
	void set(msgid_t op,handler_type *handler,bool reply = true);

	/**
	 * Unregisters the handler for <op>.
	 *
	 * @param op the operation (message-id)
	 */
	void unset(msgid_t op);

	/**
	 * Executes the device-loop, i.e. uses getwork() to get a messages and handles it with the
	 * appropriate handler.
	 */
	void loop();

	/**
	 * Calls the handler for the given message
	 *
	 * @param mid the message-id
	 * @param is the ipc-stream for the client
	 */
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
