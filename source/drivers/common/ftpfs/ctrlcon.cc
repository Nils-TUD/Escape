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
#include <vthrow.h>

#include "ctrlcon.h"

#define PRINTS		0

#if PRINTS
#	define PRINT(stmt)		(stmt)
#else
#	define PRINT(...)
#endif

const CtrlCon::Command CtrlCon::cmds[] = {
	{CMD_CONN,	"",		{220,  0}},
	{CMD_USER,	"USER",	{331,  0}},
	{CMD_PASS,	"PASS",	{230,  0}},
	{CMD_SYST,	"SYST",	{215,  0}},
	{CMD_QUIT,	"QUIT",	{221,  0}},
	{CMD_LIST,	"LIST",	{125,150}},
	{CMD_PASV,	"PASV",	{227,  0}},
	{CMD_TYPE,	"TYPE",	{200,  0}},
	{CMD_PORT,	"PORT",	{200,  0}},
	{CMD_PWD,	"PWD",	{257,  0}},
	{CMD_CWD,	"CWD",	{250,  0}},
	{CMD_CDUP,	"CDUP",	{250,  0}},
	{CMD_HELP,	"HELP",	{214,  0}},
	{CMD_SITE,	"SITE",	{200,  0}},
	{CMD_RETR,	"RETR",	{125,150}},
	{CMD_STOR,	"STOR",	{125,150}},
	{CMD_RNFR,	"RNFR",	{350,  0}},
	{CMD_RNTO,	"RNTO",	{250,  0}},
	{CMD_MKD,	"MKD",	{257,  0}},
	{CMD_RMD,	"RMD",	{250,  0}},
	{CMD_DELE,	"DELE",	{250,  0}},
	{CMD_NOOP,	"NOOP",	{200,  0}},
	{CMD_TRAN,	"TRAN",	{226,  0}},
	{CMD_ABOR,	"ABOR",	{226,  0}},
	{CMD_APPE,	"APPE",	{226,  0}},
	{CMD_REST,	"REST",	{350,  0}},
	{CMD_FEAT,	"FEAT",	{211,  0}},
	{CMD_STAT,	"STAT",	{211,  0}},
};

void CtrlCon::connect() {
	_sock = new ipc::Socket("/dev/socket",ipc::Socket::SOCK_STREAM,ipc::Socket::PROTO_TCP);
	ipc::Socket::Addr addr;
	addr.family = ipc::Socket::AF_INET;
	addr.d.ipv4.addr = _dest.value();
	addr.d.ipv4.port = _port;
	_sock->connect(addr);

	_ios.open(_sock->fd());
	readReply();

	execute(CMD_USER,_user.c_str());
	execute(CMD_PASS,_pw.c_str());
	// always use binary mode
	execute(CMD_TYPE,"I");
	if(!_dir.empty())
		execute(CMD_CWD,_dir.c_str());
}

void CtrlCon::sendCommand(const char *cmd,const char *arg) {
	PRINT(std::cerr << "[" << (void*)this << "] xmit: " << cmd << " " << arg << std::endl);
	_ios << cmd << " " << arg << "\r\n";
	_ios.flush();
}

char *CtrlCon::readLine() {
	_ios.getline(linebuf,sizeof(linebuf),'\n');
	if(_ios.bad())
		VTHROWE("Unable to read line",-ECONNRESET);

	PRINT(std::cerr << "[" << (void*)this << "] recv: " << linebuf << std::endl);
	return linebuf;
}

const char *CtrlCon::readReply() {
	while(1) {
		char *line = readLine();
		size_t len = strlen(line);
		if(len > 3 && line[3] != '-')
			return line;
	}
}

const char *CtrlCon::execute(Cmd cmd,const char *arg,bool noThrow) {
	const Command *c = cmds + cmd;

	const char *reply;
	try {
		sendCommand(c->name,arg);
		reply = readReply();
	}
	catch(const std::exception &e) {
		PRINT(std::cerr << "Command '" << c->name << " " << arg << "' failed: " << e.what() << std::endl);
		try {
			delete _sock;
			connect();

			sendCommand(c->name,arg);
			reply = readReply();
		}
		catch(...) {
			VTHROWE("Unable to send command " << cmd << " " << arg,-ECONNRESET);
		}
	}

	int code = strtoul(reply,NULL,10);
	if(code != c->success[0] && code != c->success[1]) {
		if(noThrow)
			return NULL;
		VTHROW("Got " << code << " for command '" << c->name << "'");
	}
	return reply;
}
