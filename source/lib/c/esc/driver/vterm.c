/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/driver/vterm.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <string.h>

static int vterm_doCtrl(int fd,msgid_t msgid,ulong arg1,sDataMsg *resp);

int vterm_setEcho(int fd,bool echo) {
	sDataMsg resp;
	return vterm_doCtrl(fd,echo ? MSG_VT_EN_ECHO : MSG_VT_DIS_ECHO,0,&resp);
}

int vterm_setReadline(int fd,bool readline) {
	sDataMsg resp;
	return vterm_doCtrl(fd,readline ? MSG_VT_EN_RDLINE : MSG_VT_DIS_RDLINE,0,&resp);
}

int vterm_setNavi(int fd,bool navi) {
	sDataMsg resp;
	return vterm_doCtrl(fd,navi ? MSG_VT_EN_NAVI : MSG_VT_DIS_NAVI,0,&resp);
}

int vterm_backup(int fd) {
	sDataMsg resp;
	return vterm_doCtrl(fd,MSG_VT_BACKUP,0,&resp);
}

int vterm_restore(int fd) {
	sDataMsg resp;
	return vterm_doCtrl(fd,MSG_VT_RESTORE,0,&resp);
}

int vterm_isVTerm(int fd) {
	sDataMsg resp;
	return vterm_doCtrl(fd,MSG_VT_ISVTERM,0,&resp);
}

int vterm_setMode(int fd,int mode) {
	sDataMsg resp;
	return vterm_doCtrl(fd,MSG_VT_SETMODE,mode,&resp);
}

int vterm_setShellPid(int fd,pid_t pid) {
	int res;
	sDataMsg msg;
	memcpy(&msg.d,&pid,sizeof(pid_t));
	res = send(fd,MSG_VT_SHELLPID,&msg,sizeof(msg));
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&msg,sizeof(msg)));
	if(res < 0)
		return res;
	return msg.arg1;
}

static int vterm_doCtrl(int fd,msgid_t msgid,ulong arg1,sDataMsg *resp) {
	sArgsMsg msg;
	msg.arg1 = arg1;
	int res = send(fd,msgid,&msg,sizeof(msg));
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,resp,sizeof(sDataMsg)));
	if(res < 0)
		return res;
	return resp->arg1;
}
