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
#include <esc/proto/socket.h>
#include <fstream>
#include <dns.h>

class CtrlConRef;

class CtrlCon {
	friend class CtrlConRef;

	struct Command {
		int no;
		const char *name;
		int success[2];
	};

	explicit CtrlCon(const esc::Net::IPv4Addr &ip,esc::port_t port,
			const std::string &user,const std::string &pw,const std::string &dir)
		: _refs(1), _available(false), _dest(ip), _port(port), _user(user), _pw(pw), _dir(dir),
		  _sock(), _ios() {
		connect();
	}

	CtrlCon *clone() {
		_refs++;
		return this;
	}
	void destroy() {
		if(--_refs == 0)
			delete this;
	}

	CtrlCon *request() {
		if(_available) {
			_available = false;
			return this;
		}
		return new CtrlCon(_dest,_port,_user,_pw,_dir);
	}
	void release() {
		_available = true;
	}

public:
	enum Cmd {
		CMD_CONN,
		CMD_USER,
		CMD_PASS,
		CMD_SYST,
		CMD_QUIT,
		CMD_LIST,
		CMD_PASV,
		CMD_TYPE,
		CMD_PORT,
		CMD_PWD,
		CMD_CWD,
		CMD_CDUP,
		CMD_HELP,
		CMD_SITE,
		CMD_RETR,
		CMD_STOR,
		CMD_RNFR,
		CMD_RNTO,
		CMD_MKD,
		CMD_RMD,
		CMD_DELE,
		CMD_NOOP,
		CMD_TRAN,
		CMD_ABOR,
		CMD_APPE,
		CMD_REST,
		CMD_FEAT,
		CMD_STAT,
	};

	explicit CtrlCon(const std::string &host,esc::port_t port,
			const std::string &user,const std::string &pw,const std::string &dir)
		: _refs(1), _available(true), _dest(std::DNS::getHost(host.c_str())), _port(port),
		  _user(user), _pw(pw), _dir(dir), _sock(), _ios() {
		connect();
	}
	~CtrlCon() {
		if(_sock)
			execute(CMD_QUIT,"");
		delete _sock;
	}

	const char *execute(Cmd cmd,const char *arg,bool noThrow = false);
	const char *readReply();
	void abort();

private:
	void connect();
	void sendCommand(const char *line,const char *arg);
	char *readLine();

	int _refs;
	bool _available;
	esc::Net::IPv4Addr _dest;
	esc::port_t _port;
	std::string _user;
	std::string _pw;
	std::string _dir;
	esc::Socket *_sock;
	std::fstream _ios;
	char linebuf[512];
	static const Command cmds[];
};

/**
 * A reference to the control-connection. The idea is to use the main connection whenever possible
 * and only create additional ones if it is in use (e.g. a file-transfer is in progress).
 * We do that by marking the CtrlCon object as not-available during file-transfers and clone that
 * connection if we want to do something else during that time.
 * Having an object of this class means to have a reference to the CtrlCon object in the sense that
 * it will be released and potentially free'd if this object is destructed.
 * If you want to do something with it, please use request() in order to potentially clone the
 * object. Use release() to mark it as available again. This is done automatically on object
 * destruction, too (but not twice).
 */
class CtrlConRef {
public:
	explicit CtrlConRef(CtrlCon *ctrl = NULL) : _ctrl(ctrl), _requested() {
		assert(!_ctrl || _ctrl->_refs == 1);
	}
	CtrlConRef(const CtrlConRef &r) : _ctrl(r._ctrl->clone()), _requested() {
	}
	CtrlConRef &operator=(const CtrlConRef &r) {
		if(&r != this) {
			_ctrl = r._ctrl->clone();
			_requested = false;
		}
		return *this;
	}
	~CtrlConRef() {
		if(_requested)
			_ctrl->release();
		_ctrl->destroy();
	}

	CtrlCon *get() {
		return _ctrl;
	}
	CtrlCon *request() {
		CtrlCon * nctrl = _ctrl->request();
		if(nctrl != _ctrl)
			_ctrl->destroy();
		_requested = true;
		return _ctrl = nctrl;
	}
	void release() {
		_ctrl->release();
		_requested = false;
	}

private:
	CtrlCon *_ctrl;
	bool _requested;
};
