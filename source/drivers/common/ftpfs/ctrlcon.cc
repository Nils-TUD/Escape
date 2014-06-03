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

#define PRINT_CMDS		0

#if PRINT_CMDS
#	define PRINT_CMD(stmt)		(stmt)
#else
#	define PRINT_CMD(...)
#endif

CtrlCon::Command CtrlCon::cmds[] = {
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
char CtrlCon::linebuf[512];

void CtrlCon::sendCommand(const char *cmd,const char *arg) {
	PRINT_CMD(std::cerr << "xmit: " << cmd << " " << arg << std::endl);
	_ios << cmd << " " << arg << "\r\n";
	_ios.flush();
}

char *CtrlCon::readLine() {
	_ios.getline(linebuf,sizeof(linebuf),'\n');
	PRINT_CMD(std::cerr << "recv: " << linebuf << std::endl);
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
	Command *c = cmds + cmd;
	sendCommand(c->name,arg);
	const char *reply = readReply();
	int code = strtoul(reply,NULL,10);
	if(code != c->success[0] && code != c->success[1]) {
		if(noThrow)
			return NULL;
		VTHROW("Got " << code << " for command '" << c->name << "'");
	}
	return reply;
}
