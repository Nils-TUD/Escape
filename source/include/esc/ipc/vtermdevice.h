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
#include <esc/ipc/requestqueue.h>
#include <esc/proto/file.h>
#include <esc/proto/vterm.h>
#include <sys/common.h>
#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include <list>
#include <stdlib.h>

namespace esc {

/**
 * The base-class for all virtual-terminal devices. Supports all vterm-operations, but not the UI-
 * and Screen-operations (should be added by the subclass).
 */
class VTermDevice : public Device {
protected:
	static const size_t BUF_SIZE	= 4096;

public:
	/**
	 * Creates the device at given path.
	 *
	 * @param path the path
	 * @param mode the permissions to set
	 * @param vterm the vterm-instance
	 * @throws if the operation failed
	 */
	explicit VTermDevice(const char *name,mode_t mode,sVTerm *vterm)
		: Device(name,mode,DEV_TYPE_CHAR,DEV_CANCEL | DEV_READ | DEV_WRITE | DEV_CLOSE),
		  _requests(std::make_memfun(this,&VTermDevice::handleRead)), _vterm(vterm) {
		set(MSG_DEV_CANCEL,std::make_memfun(this,&VTermDevice::cancel));
		set(MSG_FILE_READ,std::make_memfun(this,&VTermDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&VTermDevice::write));
		set(MSG_VT_GETFLAG,std::make_memfun(this,&VTermDevice::getFlag));
		set(MSG_VT_SETFLAG,std::make_memfun(this,&VTermDevice::setFlag));
		set(MSG_VT_BACKUP,std::make_memfun(this,&VTermDevice::backup));
		set(MSG_VT_RESTORE,std::make_memfun(this,&VTermDevice::restore));
		set(MSG_VT_SHELLPID,std::make_memfun(this,&VTermDevice::setShellPid));
		set(MSG_VT_ISVTERM,std::make_memfun(this,&VTermDevice::isVTerm));
		set(MSG_VT_SETMODE,std::make_memfun(this,&VTermDevice::setMode));
	}

	/**
	 * Sets the video mode to <mode>. Has to be implemented by the subclass.
	 *
	 * @param mode the video mode
	 */
	virtual void setVideoMode(int mode) = 0;
	/**
	 * Should update the display in case something changed. Has to be implemented by the subclass.
	 */
	virtual void update() = 0;

	/**
	 * Checks whether there are pending read-requests to be served and does so, if there are some.
	 */
	void checkPending() {
		_requests.handle();
	}

private:
	void cancel(IPCStream &is) {
		DevCancel::Request r;
		is >> r;

		errcode_t res;
		// we answer write-requests always right away, so let the kernel just wait for the response
		if(r.msg == MSG_FILE_WRITE)
			res = DevCancel::READY;
		else if(r.msg != MSG_FILE_READ)
			res = -EINVAL;
		else {
			std::lock_guard<std::mutex> guard(*_vterm->mutex);
			if(vtin_hasData(_vterm))
				res = DevCancel::READY;
			else
				res = _requests.cancel(r.mid);
		}
		is << DevCancel::Response(res) << Reply();
	}

	void read(IPCStream &is) {
		FileRead::Request r;
		is >> r;
		assert(!is.error());

		char *data = r.count <= BUF_SIZE ? _buffer : new char[r.count];

		std::lock_guard<std::mutex> guard(*_vterm->mutex);
		if(_requests.size() == 0 && vtin_hasData(_vterm))
			handleRead(is.fd(),is.msgid(),data,r.count);
		else
			_requests.enqueue(Request(is.fd(),is.msgid(),data,r.count));
	}

	void write(IPCStream &is) {
		FileWrite::Request r;
		is >> r;
		assert(!is.error());

		DataBuf buf(r.count < BUF_SIZE ? _buffer : new char[r.count + 1],r.count >= BUF_SIZE);

		is >> ReceiveData(buf.data(),r.count);
		buf.data()[r.count] = '\0';

		{
			std::lock_guard<std::mutex> guard(*_vterm->mutex);
			vtout_puts(_vterm,buf.data(),r.count,true);
			update();
		}

		is << FileWrite::Response::success(r.count) << Reply();

		std::lock_guard<std::mutex> guard(*_vterm->mutex);
		checkPending();
	}

	void setMode(esc::IPCStream &is) {
		int mode;
		is >> mode;
		{
			std::lock_guard<std::mutex> guard(*_vterm->mutex);
			setVideoMode(mode);
		}
		is << errcode_t(0) << esc::Reply();
	}

	void getFlag(IPCStream &is) {
		VTerm::Flag flag;
		is >> flag;
		errcode_t res = handleControlMsg(MSG_VT_GETFLAG,flag,0);
		is << ValueResponse<bool>::result(res) << Reply();
	}
	void setFlag(IPCStream &is) {
		bool val;
		VTerm::Flag flag;
		is >> flag >> val;
		errcode_t res = handleControlMsg(MSG_VT_SETFLAG,flag,val);
		is << res << Reply();
	}
	void backup(IPCStream &is) {
		errcode_t res = handleControlMsg(MSG_VT_BACKUP,0,0);
		is << res << Reply();
	}
	void restore(IPCStream &is) {
		errcode_t res = handleControlMsg(MSG_VT_RESTORE,0,0);
		is << res << Reply();
	}
	void setShellPid(IPCStream &is) {
		pid_t pid;
		is >> pid;
		errcode_t res = handleControlMsg(MSG_VT_SHELLPID,pid,0);
		is << res << Reply();
	}
	void isVTerm(IPCStream &is) {
		errcode_t res = handleControlMsg(MSG_VT_ISVTERM,0,0);
		is << res << Reply();
	}

	errcode_t handleControlMsg(msgid_t mid,int arg1,int arg2) {
		errcode_t res = vtctrl_control(_vterm,mid,arg1,arg2);
		{
			std::lock_guard<std::mutex> guard(*_vterm->mutex);
			checkPending();
			update();
		}
		return res;
	}

protected:
	bool handleRead(int fd,msgid_t mid,char *data,size_t count) {
		if(!vtin_hasData(_vterm))
			return false;

		ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
		IPCStream is(fd,buffer,sizeof(buffer),mid);

		ssize_t res = vtin_gets(_vterm,data,count);
		is << FileRead::Response::result(res) << Reply();
		if(res > 0)
			is << ReplyData(data,res);
		if(count > BUF_SIZE)
			delete[] data;
		return true;
	}

	char _buffer[BUF_SIZE];
	RequestQueue _requests;
	sVTerm *_vterm;
};

}
