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
#include <gui/common.h>
#include <esc/messages.h>
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/esccodes.h>
#include <esc/thread.h>
#include <signal.h>
#include <errors.h>

#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include "shellapp.h"

ShellApplication::ShellApplication(tFD sid,ShellControl *sh)
		: Application(), _sid(sid), _sh(sh), _cfg(sVTermCfg()),
		  rbuffer(new char[READ_BUF_SIZE]), rbufPos(0) {
	_inst = this;
	/* no blocking here since we want to check multiple things */
	fcntl(_winFd,F_SETFL,IO_NOBLOCK);

	_waits[0].events = EV_RECEIVED_MSG;
	_waits[0].object = _winFd;
	_waits[1].events = EV_CLIENT;
	_waits[1].object = _sid;
}

ShellApplication::~ShellApplication() {
	delete rbuffer;
}

void ShellApplication::doEvents() {
	tMsgId mid;
	ssize_t res = RETRY(receive(_winFd,&mid,&_msg,sizeof(_msg)));
	if(res < 0 && res != ERR_WOULD_BLOCK)
		error("Read from window-manager failed");
	if(res > 0) {
		switch(mid) {
			case MSG_WIN_KEYBOARD:
				handleKbMsg();
				break;

			default:
				handleMessage(mid,&_msg);
				break;
		}
	}
	else
		driverMain();
}

void ShellApplication::handleKbMsg() {
	uchar keycode = (uchar)_msg.args.arg1;
	bool isBreak = (bool)_msg.args.arg2;
	char character = (char)_msg.args.arg4;
	uchar modifier = (uchar)_msg.args.arg5;
	if(isBreak)
		return;

	vterm_handleKey(_sh->getVTerm(),keycode,((modifier & SHIFT_MASK) ? STATE_SHIFT : 0) |
			((modifier & ALT_MASK) ? STATE_ALT : 0) |
			((modifier & CTRL_MASK) ? STATE_CTRL : 0),character);
	_sh->update();
}

void ShellApplication::driverMain() {
	tMsgId mid;
	tFD fd = getWork(&_sid,1,NULL,&mid,&_msg,sizeof(_msg),GW_NOBLOCK);
	if(fd < 0) {
		if(fd != ERR_NO_CLIENT_WAITING)
			printe("[GUISH] Unable to get client");
		// append the buffer now to reduce delays
		if(rbufPos > 0) {
			rbuffer[rbufPos] = '\0';
			vterm_puts(_sh->getVTerm(),rbuffer,rbufPos,true);
			_sh->update();
			rbufPos = 0;
		}
		waitm(_waits,ARRAY_SIZE(_waits));
	}
	else {
		switch(mid) {
			case MSG_DRV_READ: {
				sVTerm *vt = _sh->getVTerm();
				sRingBuf *inbuf = _sh->getInBuf();
				// offset is ignored here
				size_t count = _msg.args.arg2;
				char *data = (char*)malloc(count);
				_msg.args.arg1 = 0;
				if(data)
					_msg.args.arg1 = rb_readn(inbuf,data,count);
				_msg.args.arg2 = vt->inbufEOF || rb_length(inbuf) > 0;
				if(rb_length(inbuf) == 0)
					vt->inbufEOF = false;
				send(fd,MSG_DRV_READ_RESP,&_msg,sizeof(_msg.args));
				if(data) {
					send(fd,MSG_DRV_READ_RESP,data,count);
					free(data);
				}
			}
			break;

			case MSG_DRV_WRITE: {
				size_t amount;
				char *data;
				size_t c = _msg.args.arg2;
				data = (char*)malloc(c + 1);
				_msg.args.arg1 = 0;
				if(data) {
					if(RETRY(receive(fd,&mid,data,c + 1)) >= 0) {
						char *dataWork = data;
						_msg.args.arg1 = c;
						dataWork[c] = '\0';
						while(c > 0) {
							amount = MIN(c,READ_BUF_SIZE - rbufPos);
							memcpy(rbuffer + rbufPos,dataWork,amount);

							c -= amount;
							rbufPos += amount;
							dataWork += amount;
							if(rbufPos >= READ_BUF_SIZE) {
								rbuffer[rbufPos] = '\0';
								vterm_puts(_sh->getVTerm(),rbuffer,rbufPos,true);
								rbufPos = 0;
							}
						}
					}
					free(data);
				}
				send(fd,MSG_DRV_WRITE_RESP,&_msg,sizeof(_msg.args));
			}
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
				_msg.data.arg1 = vterm_ctl(_sh->getVTerm(),&_cfg,mid,_msg.data.d);
				send(fd,MSG_DEF_RESPONSE,&_msg,sizeof(_msg.data));
				break;
		}
		_sh->update();
		close(fd);
	}
}
