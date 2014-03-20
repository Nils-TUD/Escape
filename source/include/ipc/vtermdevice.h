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
#include <ipc/ipcstream.h>
#include <ipc/device.h>
#include <ipc/proto/device.h>
#include <ipc/proto/vterm.h>
#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include <stdlib.h>

namespace ipc {

class VTermDevice : public Device {
protected:
	static const size_t BUF_SIZE	= 4096;

public:
	explicit VTermDevice(const char *name,mode_t mode,sVTerm *vterm)
		: Device(name,mode,DEV_TYPE_CHAR,DEV_READ | DEV_WRITE | DEV_CLOSE), _vterm(vterm) {
		set(MSG_DEV_READ,std::make_memfun(this,&VTermDevice::read));
		set(MSG_DEV_WRITE,std::make_memfun(this,&VTermDevice::write));
		set(MSG_VT_GETFLAG,std::make_memfun(this,&VTermDevice::getFlag));
		set(MSG_VT_SETFLAG,std::make_memfun(this,&VTermDevice::setFlag));
		set(MSG_VT_BACKUP,std::make_memfun(this,&VTermDevice::backup));
		set(MSG_VT_RESTORE,std::make_memfun(this,&VTermDevice::restore));
		set(MSG_VT_SHELLPID,std::make_memfun(this,&VTermDevice::setShellPid));
		set(MSG_VT_ISVTERM,std::make_memfun(this,&VTermDevice::isVTerm));
		set(MSG_VT_SETMODE,std::make_memfun(this,&VTermDevice::setMode));
	}

	void read(IPCStream &is) {
		DevRead::Request r;
		is >> r;
		assert(!is.error());

		ssize_t res = 0;
		char *data = r.count <= BUF_SIZE ? _buffer : (char*)malloc(r.count);
		if(data) {
			int avail;
			res = vtin_gets(_vterm,data,r.count,&avail);
		}

		is << DevRead::Response(res) << Send(DevRead::Response::MID);
		if(res) {
			is << SendData(DevRead::Response::MID,data,res);
			if(r.count > BUF_SIZE)
				free(data);
		}
	}

	void write(IPCStream &is) {
		DevWrite::Request r;
		is >> r;
		assert(!is.error());

		char *data = r.count <= BUF_SIZE ? _buffer : (char*)malloc(r.count + 1);
		ssize_t res = 0;
		if(data) {
			is >> ReceiveData(data,r.count);
			data[r.count] = '\0';
			vtout_puts(_vterm,data,r.count,true);
			update();
			res = r.count;
			if(r.count > BUF_SIZE)
				free(data);
		}

		is << DevWrite::Response(res) << Send(DevWrite::Response::MID);
	}

	void setMode(ipc::IPCStream &is) {
		int mode;
		is >> mode;
		setVideoMode(mode);
		is << 0 << ipc::Send(MSG_DEF_RESPONSE);
	}

	void getFlag(IPCStream &is) {
		VTerm::Flag flag;
		is >> flag;
		handleControlMsg(is,MSG_VT_GETFLAG,flag,0);
	}
	void setFlag(IPCStream &is) {
		bool val;
		VTerm::Flag flag;
		is >> flag >> val;
		handleControlMsg(is,MSG_VT_SETFLAG,flag,val);
	}
	void backup(IPCStream &is) {
		handleControlMsg(is,MSG_VT_BACKUP,0,0);
	}
	void restore(IPCStream &is) {
		handleControlMsg(is,MSG_VT_RESTORE,0,0);
	}
	void setShellPid(IPCStream &is) {
		int pid;
		is >> pid;
		handleControlMsg(is,MSG_VT_SHELLPID,pid,0);
	}
	void isVTerm(IPCStream &is) {
		handleControlMsg(is,MSG_VT_ISVTERM,0,0);
	}

private:
	virtual void setVideoMode(int mode) = 0;
	virtual void update() = 0;

	void handleControlMsg(IPCStream &is,msgid_t mid,int arg1,int arg2) {
		int res = vtctrl_control(_vterm,mid,arg1,arg2);
		update();
		is << res << Send(MSG_DEF_RESPONSE);
	}

protected:
	char _buffer[BUF_SIZE];
	sVTerm *_vterm;
};

}
