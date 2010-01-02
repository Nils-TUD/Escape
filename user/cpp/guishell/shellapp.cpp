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
#include <errors.h>
#include "shellapp.h"

ShellApplication::ShellApplication(tServ sid,u32 no,ShellControl *sh)
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

	switch(keycode) {
		case VK_PGUP:
			_sh->scrollPage(1);
			break;
		case VK_PGDOWN:
			_sh->scrollPage(-1);
			break;

		case VK_END:
		case VK_HOME:
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
		case VK_D:
		case VK_C:
			if(modifier & SHIFT_MASK) {
				switch(keycode) {
					case VK_UP:
						_sh->scrollLine(1);
						break;
					case VK_DOWN:
						_sh->scrollLine(-1);
						break;
				}
				break;
			}
			if(modifier & CTRL_MASK) {
				switch(keycode) {
					case VK_C:
						// don't send it to the vterms
						sendSignal(SIG_INTRPT,0xFFFFFFFF);
						break;
					case VK_D: {
						char eof = IO_EOF;
						_sh->addToInBuf(&eof,1);
					}
					break;
				}
				break;
			}
			// fall through

		default:
			if(_sh->getReadLine()) {
				if(modifier & CTRL_MASK) {
					if(_sh->getEcho())
						_sh->rlHandleKeycode(keycode);
				}
				else if(character)
					_sh->rlPutchar(character);
			}
			else {
				char escape[SSTRLEN("\033[kc;123;123;7]") + 1];
				sprintf(escape,"\033[kc;%d;%d;%d]",character,keycode,modifier);
				_sh->addToInBuf(escape,strlen(escape));
			}
			break;
	}
}

void ShellApplication::driverMain() {
	tMsgId mid;
	tServ client;
	tFD fd = getClient(&_sid,1,&client);
	if(fd < 0) {
		// append the buffer now to reduce delays
		if(rbufPos > 0) {
			_sh->append(rbuffer,rbufPos);
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
								_sh->append(rbuffer,rbufPos);
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
					bool readKeyboard; // TODO
					_msg.data.arg1 = _sh->ioctl(_msg.data.arg1,_msg.data.d,&readKeyboard);
					send(fd,MSG_DRV_IOCTL_RESP,&_msg,sizeof(_msg.data));
				}
				break;
				case MSG_DRV_CLOSE:
					break;
			}
		}
		close(fd);
	}
}
