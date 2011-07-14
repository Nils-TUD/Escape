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
#include <vterm/vtout.h>
#include <esc/driver.h>
#include <esc/thread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errors.h>
#include "guiterm.h"

GUITerm::GUITerm(tULock *lock,int sid,ShellControl *sh)
	: _sid(sid), _run(true), _vt(NULL), _sh(sh), _lock(lock), _cfg(sVTermCfg()),
	  _rbuffer(new char[READ_BUF_SIZE]), _rbufPos(0) {
	sVTSize size;
	size.width = sh->getCols();
	size.height = sh->getRows();

	// open speaker
	int speakerFd = open("/dev/speaker",IO_MSGS);
	if(speakerFd < 0)
		error("Unable to open '/dev/speaker'");

	_vt = (sVTerm*)malloc(sizeof(sVTerm));
	if(!_vt)
		error("Not enough mem for vterm");
	_vt->index = 0;
	_vt->sid = sid;
	_vt->defForeground = BLACK;
	_vt->defBackground = WHITE;
	if(getenvto(_vt->name,sizeof(_vt->name),"TERM") < 0)
		error("Unable to get env-var TERM");
	if(!vterm_init(_vt,&size,-1,speakerFd))
		error("Unable to init vterm");
	_vt->active = true;
	_sh->setVTerm(_vt);
}

GUITerm::~GUITerm() {
	delete[] _rbuffer;
	vterm_destroy(_vt);
	free(_vt);
}

void GUITerm::run() {
	sMsg msg;
	msgid_t mid;
	while(_run) {
		int fd = getWork(&_sid,1,NULL,&mid,&msg,sizeof(msg),GW_NOBLOCK);
		if(fd < 0) {
			if(fd != ERR_NO_CLIENT_WAITING)
				printe("[GUISH] Unable to get client");
			// append the buffer now to reduce delays
			if(_rbufPos > 0) {
				_rbuffer[_rbufPos] = '\0';
				locku(_lock);
				vterm_puts(_vt,_rbuffer,_rbufPos,true);
				_sh->update();
				unlocku(_lock);
				_rbufPos = 0;
			}
			wait(EV_CLIENT,_sid);
		}
		else {
			switch(mid) {
				case MSG_DRV_READ:
					read(fd,&msg);
					break;

				case MSG_DRV_WRITE:
					write(fd,&msg);
					break;

				case MSG_VT_SHELLPID:
				case MSG_VT_ENABLE:
				case MSG_VT_DISABLE:
				case MSG_VT_EN_ECHO:
				case MSG_VT_DIS_ECHO:
				case MSG_VT_EN_RDLINE:
				case MSG_VT_DIS_RDLINE:
				case MSG_VT_EN_RDKB:
				case MSG_VT_DIS_RDKB:
				case MSG_VT_EN_NAVI:
				case MSG_VT_DIS_NAVI:
				case MSG_VT_BACKUP:
				case MSG_VT_RESTORE:
				case MSG_VT_GETSIZE:
					locku(_lock);
					msg.data.arg1 = vterm_ctl(_vt,&_cfg,mid,msg.data.d);
					unlocku(_lock);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
					break;
			}
			locku(_lock);
			_sh->update();
			unlocku(_lock);
			close(fd);
		}
	}
}

void GUITerm::read(int fd,sMsg *msg) {
	// offset is ignored here
	size_t count = msg->args.arg2;
	char *data = (char*)malloc(count);
	msg->args.arg1 = 0;
	locku(_lock);
	if(data)
		msg->args.arg1 = rb_readn(_vt->inbuf,data,count);
	if(rb_length(_vt->inbuf) == 0)
		_vt->inbufEOF = false;
	msg->args.arg2 = _vt->inbufEOF || rb_length(_vt->inbuf) > 0;
	unlocku(_lock);
	send(fd,MSG_DRV_READ_RESP,msg,sizeof(msg->args));
	if(data) {
		send(fd,MSG_DRV_READ_RESP,data,count);
		free(data);
	}
}

void GUITerm::write(int fd,sMsg *msg) {
	msgid_t mid;
	size_t amount;
	size_t c = msg->args.arg2;
	char *data = (char*)malloc(c + 1);
	msg->args.arg1 = 0;
	if(data) {
		if(RETRY(receive(fd,&mid,data,c + 1)) >= 0) {
			char *dataWork = data;
			msg->args.arg1 = c;
			dataWork[c] = '\0';
			while(c > 0) {
				amount = MIN(c,READ_BUF_SIZE - _rbufPos);
				memcpy(_rbuffer + _rbufPos,dataWork,amount);

				c -= amount;
				_rbufPos += amount;
				dataWork += amount;
				if(_rbufPos >= READ_BUF_SIZE) {
					_rbuffer[_rbufPos] = '\0';
					locku(_lock);
					vterm_puts(_vt,_rbuffer,_rbufPos,true);
					unlocku(_lock);
					_rbufPos = 0;
				}
			}
		}
		free(data);
	}
	send(fd,MSG_DRV_WRITE_RESP,msg,sizeof(msg->args));
}
