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

#include <esc/common.h>
#include <esc/io.h>
#include <esc/messages.h>
#include <errors.h>
#include <string.h>
#include <stdarg.h>

bool is_file(const char *path) {
	sFileInfo info;
	if(stat(path,&info) < 0)
		return false;
	return MODE_IS_FILE(info.mode);
}

bool is_dir(const char *path) {
	sFileInfo info;
	if(stat(path,&info) < 0)
		return false;
	return MODE_IS_DIR(info.mode);
}

ssize_t sendRecvMsgData(tFD fd,tMsgId id,const void *data,size_t size) {
	tMsgId mid;
	sMsg msg;
	int res;
	if(data) {
		memcpy(msg.data.d,data,size);
		if((res = send(fd,id,&msg,sizeof(msg.data))) < 0)
			return res;
	}
	else {
		if((res = send(fd,id,NULL,0)) < 0)
			return res;
	}
	if((res = receive(fd,&mid,&msg,sizeof(msg))) < 0)
		return res;
	return (ssize_t)msg.args.arg1;
}

ssize_t sendMsgData(tFD fd,tMsgId id,const void *data,size_t size) {
	sMsg msg;
	if(size > sizeof(msg.data.d))
		return ERR_NOT_ENOUGH_MEM;
	memcpy(msg.data.d,data,size);
	return send(fd,id,&msg,sizeof(msg.data));
}

ssize_t recvMsgData(tFD fd,tMsgId id,void *data,size_t size) {
	sMsg msg;
	ssize_t res;
	if((res = send(fd,id,NULL,0)) < 0)
		return res;
	if((res = RETRY(receive(fd,NULL,&msg,sizeof(msg)))) < 0)
		return res;
	res = (ssize_t)msg.data.arg1;
	if(res > (ssize_t)size)
		res = size;
	if(res > 0)
		memcpy(data,msg.data.d,res);
	return res;
}

ssize_t vrecvMsgData(tFD fd,tMsgId id,void *data,size_t size,size_t argc,...) {
	sMsg msg;
	ssize_t res;
	va_list ap;
	va_start(ap,argc);
	msg.args.arg1 = argc >= 1 ? va_arg(ap,uint) : 0;
	msg.args.arg2 = argc >= 2 ? va_arg(ap,uint) : 0;
	msg.args.arg3 = argc >= 3 ? va_arg(ap,uint) : 0;
	msg.args.arg4 = argc >= 4 ? va_arg(ap,uint) : 0;
	msg.args.arg5 = argc >= 5 ? va_arg(ap,uint) : 0;
	msg.args.arg6 = argc >= 6 ? va_arg(ap,uint) : 0;
	va_end(ap);
	if((res = send(fd,id,&msg,sizeof(msg.args))) < 0)
		return res;
	if((res = RETRY(receive(fd,NULL,&msg,sizeof(msg)))) < 0)
		return res;
	res = (ssize_t)msg.data.arg1;
	if(res > (ssize_t)size)
		res = size;
	if(res > 0)
		memcpy(data,msg.data.d,res);
	return res;
}
