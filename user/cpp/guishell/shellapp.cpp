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

	// create input-buffer
	_inbuf = rb_create(sizeof(char),GUISH_INBUF_SIZE,RB_OVERWRITE);
	if(_inbuf == NULL) {
		printe("Unable to create ring-buffer");
		exit(EXIT_FAILURE);
	}

	// init read-buffer
	rbuffer = new char[READ_BUF_SIZE];
	rbufPos = 0;
}

ShellApplication::~ShellApplication() {
	delete rbuffer;
}

void ShellApplication::doEvents() {
	u32 c;
	tMsgId mid;
	if(!eof(_winFd)) {
		if(receive(_winFd,&mid,&_msg) < 0) {
			printe("Read from window-manager failed");
			exit(EXIT_FAILURE);
		}

		switch(mid) {
			case MSG_WIN_KEYBOARD: {
				u8 keycode = (u8)_msg.args.arg1;
				bool isBreak = (bool)_msg.args.arg2;
				char character = (char)_msg.args.arg4;
				u8 modifier = (u8)_msg.args.arg5;
				if(isBreak)
					break;

				switch(keycode) {
					case VK_PGUP:
						_sh->scrollPage(1);
						break;
					case VK_PGDOWN:
						_sh->scrollPage(-1);
						break;

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
									putIn(&eof,1);
								}
								break;
							}
							break;
						}
						// fall through

					default:
						if(character)
							putIn(&character,1);
						else {
							char str[] = {'\033',keycode,modifier};
							putIn(str,3);
						}
						break;
				}
			}
			break;

			default:
				handleMessage(mid,&_msg);
				break;
		}
	}
	else {
		tServ client;
		tFD fd = getClient(&_sid,1,&client);
		if(fd < 0) {
			// append the buffer now to reduce delays
			if(rbufPos > 0) {
				_sh->append(rbuffer);
				rbufPos = 0;
			}
			wait(EV_CLIENT | EV_RECEIVED_MSG);
		}
		else {
			u32 x = 0;
			while(receive(fd,&mid,&_msg) > 0) {
				switch(mid) {
					case MSG_DRV_OPEN:
						_msg.args.arg1 = 0;
						send(fd,MSG_DRV_OPEN_RESP,&_msg,sizeof(_msg.args));
						break;
					case MSG_DRV_READ: {
						/* offset is ignored here */
						u32 count = MIN(sizeof(_msg.data.d),_msg.args.arg2);
						_msg.data.arg1 = rb_readn(_inbuf,_msg.data.d,count);
						_msg.data.arg2 = rb_length(_inbuf) > 0;
						send(fd,MSG_DRV_READ_RESP,&_msg,sizeof(_msg.data));
					}
					break;
					case MSG_DRV_WRITE: {
						u32 amount;
						char *input;
						c = _msg.data.arg2;
						if(c >= sizeof(_msg.data.d))
							c = sizeof(_msg.data.d) - 1;
						_msg.data.d[c] = '\0';
						_msg.args.arg1 = c;

						input = _msg.data.d;
						while(c > 0) {
							amount = MIN(c,READ_BUF_SIZE);
							memcpy(rbuffer + rbufPos,input,amount);

							c -= amount;
							rbufPos += amount;
							if(rbufPos >= READ_BUF_SIZE) {
								_sh->append(rbuffer);
								rbufPos = 0;
							}
						}

						send(fd,MSG_DRV_WRITE_RESP,&_msg,sizeof(_msg.args));
					}
					break;
					case MSG_DRV_IOCTL: {
						_msg.data.arg1 = ERR_UNSUPPORTED_OPERATION;
						_msg.data.arg2 = 0;
						send(fd,MSG_DRV_IOCTL_RESP,&_msg,sizeof(_msg.data));
					}
					break;
					case MSG_DRV_CLOSE:
						break;
				}
			}
		}
	}
}

void ShellApplication::putIn(char *s,u32 len) {
	if(rb_length(_inbuf) == 0)
		setDataReadable(_sid,true);
	rb_writen(_inbuf,s,len);
}
