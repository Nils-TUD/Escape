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
#include <messages.h>
#include <esc/service.h>
#include <esc/io.h>
#include <esc/signals.h>
#include <esc/gui/common.h>
#include <esccodes.h>
#include <errors.h>

#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include "shellapp.h"

ShellApplication::ShellApplication(tServ sid,ShellControl *sh)
		: Application(), _sid(sid), _sh(sh) {
	_inst = this;

	// init read-buffer
	rbuffer = new char[READ_BUF_SIZE];
	rbufPos = 0;
}

ShellApplication::~ShellApplication() {
	delete rbuffer;
}

void ShellApplication::doEvents() {
	tMsgId mid;
	if(!eof(_winFd)) {
		if(receive(_winFd,&mid,&_msg,sizeof(_msg)) < 0) {
			printe("Read from window-manager failed");
			exit(EXIT_FAILURE);
		}

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
	u8 keycode = (u8)_msg.args.arg1;
	bool isBreak = (bool)_msg.args.arg2;
	char character = (char)_msg.args.arg4;
	u8 modifier = (u8)_msg.args.arg5;
	if(isBreak)
		return;

	vterm_handleKey(_sh->getVTerm(),keycode,((modifier & SHIFT_MASK) ? STATE_SHIFT : 0) |
			((modifier & ALT_MASK) ? STATE_ALT : 0) |
			((modifier & CTRL_MASK) ? STATE_CTRL : 0),character);
	_sh->update();
}

void ShellApplication::driverMain() {
	tMsgId mid;
	tServ client;
	tFD fd = getClient(&_sid,1,&client);
	if(fd < 0) {
		if(fd != ERR_NO_CLIENT_WAITING)
			printe("Unable to get client");
		// append the buffer now to reduce delays
		if(rbufPos > 0) {
			rbuffer[rbufPos] = '\0';
			vterm_puts(_sh->getVTerm(),rbuffer,rbufPos,true);
			_sh->update();
			rbufPos = 0;
		}
		wait(EV_CLIENT | EV_RECEIVED_MSG);
	}
	else {
		while(receive(fd,&mid,&_msg,sizeof(_msg)) > 0) {
			switch(mid) {
				case MSG_DRV_OPEN:
					_msg.args.arg1 = 0;
					send(fd,MSG_DRV_OPEN_RESP,&_msg,sizeof(_msg.args));
					break;
				case MSG_DRV_READ: {
					sRingBuf *inbuf = _sh->getInBuf();
					// offset is ignored here
					u32 count = _msg.args.arg2;
					char *data = (char*)malloc(count);
					_msg.args.arg1 = 0;
					if(data)
						_msg.args.arg1 = rb_readn(inbuf,data,count);
					_msg.args.arg2 = rb_length(inbuf) > 0;
					send(fd,MSG_DRV_READ_RESP,&_msg,sizeof(_msg.args));
					if(data) {
						send(fd,MSG_DRV_READ_RESP,data,count);
						free(data);
					}
				}
				break;
				case MSG_DRV_WRITE: {
					u32 amount;
					char *data;
					u32 c = _msg.args.arg2;
					data = (char*)malloc(c + 1);
					_msg.args.arg1 = 0;
					if(data) {
						receive(fd,&mid,data,c + 1);
						data[c] = '\0';
						while(c > 0) {
							amount = MIN(c,READ_BUF_SIZE - rbufPos);
							memcpy(rbuffer + rbufPos,data,amount);

							c -= amount;
							rbufPos += amount;
							data += amount;
							if(rbufPos >= READ_BUF_SIZE) {
								rbuffer[rbufPos] = '\0';
								vterm_puts(_sh->getVTerm(),rbuffer,rbufPos,true);
								rbufPos = 0;
							}
						}
						free(data);
						_msg.args.arg1 = c;
					}
					send(fd,MSG_DRV_WRITE_RESP,&_msg,sizeof(_msg.args));
				}
				break;
				case MSG_DRV_IOCTL: {
					bool readKeyboard;
					_msg.data.arg1 = vterm_ioctl(
							_sh->getVTerm(),_msg.data.arg1,_msg.data.d,&readKeyboard);
					send(fd,MSG_DRV_IOCTL_RESP,&_msg,sizeof(_msg.data));
				}
				break;
				case MSG_DRV_CLOSE:
					break;
			}
		}
		_sh->update();
		close(fd);
	}
}
