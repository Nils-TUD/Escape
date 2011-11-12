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

static int vterm_doCtrl(int fd,msgid_t msg);

int vterm_setEnabled(int fd,bool enabled) {
	return vterm_doCtrl(fd,enabled ? MSG_VT_ENABLE : MSG_VT_DISABLE);
}

int vterm_setEcho(int fd,bool echo) {
	return vterm_doCtrl(fd,echo ? MSG_VT_EN_ECHO : MSG_VT_DIS_ECHO);
}

int vterm_setReadline(int fd,bool readline) {
	return vterm_doCtrl(fd,readline ? MSG_VT_EN_RDLINE : MSG_VT_DIS_RDLINE);
}

int vterm_setReadKB(int fd,bool enRead) {
	return vterm_doCtrl(fd,enRead ? MSG_VT_EN_RDKB : MSG_VT_DIS_RDKB);
}

int vterm_setNavi(int fd,bool navi) {
	return vterm_doCtrl(fd,navi ? MSG_VT_EN_NAVI : MSG_VT_DIS_NAVI);
}

int vterm_backup(int fd) {
	return vterm_doCtrl(fd,MSG_VT_BACKUP);
}

int vterm_restore(int fd) {
	return vterm_doCtrl(fd,MSG_VT_RESTORE);
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

int vterm_getSize(int fd,sVTSize *size) {
	sDataMsg resp;
	int res = send(fd,MSG_VT_GETSIZE,NULL,0);
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&resp,sizeof(resp)));
	if(res < 0)
		return res;
	if(resp.arg1 == sizeof(sVTSize))
		memcpy(size,&resp.d,resp.arg1);
	return resp.arg1;
}

int vterm_select(int fd,int vterm) {
	sArgsMsg msg;
	msg.arg1 = vterm;
	return send(fd,MSG_VT_SELECT,&msg,sizeof(msg));
}

static int vterm_doCtrl(int fd,msgid_t msg) {
	sDataMsg resp;
	int res = send(fd,msg,NULL,0);
	if(res < 0)
		return res;
	res = IGNSIGS(receive(fd,NULL,&resp,sizeof(resp)));
	if(res < 0)
		return res;
	return resp.arg1;
}
