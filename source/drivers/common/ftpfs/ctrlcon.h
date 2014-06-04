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

#include <esc/common.h>
#include <ipc/proto/socket.h>
#include <fstream>
#include <dns.h>

class CtrlCon {
	struct Command {
		int no;
		const char *name;
		int success[2];
	};

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

	explicit CtrlCon(const std::string &host,ipc::port_t port,
			const std::string &user,const std::string &pw)
		: _dest(std::DNS::getHost(host.c_str())), _port(port), _user(user), _pw(pw), _sock(), _ios() {
		connect();
	}
	~CtrlCon() {
		if(_sock)
			execute(CMD_QUIT,"");
		delete _sock;
	}

	const char *execute(Cmd cmd,const char *arg,bool noThrow = false);
	const char *readReply();

private:
	void connect();
	void sendCommand(const char *line,const char *arg);
	char *readLine();

	ipc::Net::IPv4Addr _dest;
	ipc::port_t _port;
	std::string _user;
	std::string _pw;
	ipc::Socket *_sock;
	std::fstream _ios;
	static Command cmds[];
	static char linebuf[];
};
