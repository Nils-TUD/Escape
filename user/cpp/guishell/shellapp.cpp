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
#include <esc/messages.h>
#include <esc/service.h>
#include <esc/io.h>
#include <esc/signals.h>
#include <esc/gui/common.h>
#include "shellapp.h"

void ShellApplication::doEvents() {
	sMsgHeader header;
	u32 c;
	if(!eof(_winFd)) {
		if((c = read(_winFd,&header,sizeof(header))) != sizeof(header)) {
			printe("Read from window-manager failed; Read %d bytes, expected %d",
					c,sizeof(header));
			exit(EXIT_FAILURE);
		}

		switch(header.id) {
			case MSG_WIN_KEYBOARD: {
				sMsgDataWinKeyboard data;
				if(read(_winFd,&data,sizeof(data)) == sizeof(data)) {
					if(!data.isBreak) {
						switch(data.keycode) {
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
								if(data.modifier & SHIFT_MASK) {
									switch(data.keycode) {
										case VK_UP:
											_sh->scrollLine(1);
											break;
										case VK_DOWN:
											_sh->scrollLine(-1);
											break;
									}
									break;
								}
								if(data.modifier & CTRL_MASK) {
									switch(data.keycode) {
										case VK_C:
											// don't send it to the vterms
											sendSignal(SIG_INTRPT,0xFFFFFFFF);
											break;
										case VK_D: {
											char eof = IO_EOF;
											write(_selfFd,&eof,sizeof(eof));
										}
										break;
									}
									break;
								}
								// fall through

							default:
								if(data.character)
									write(_selfFd,&data.character,sizeof(data.character));
								else {
									char str[] = {'\033',data.keycode,data.modifier};
									write(_selfFd,str,sizeof(str));
								}
								break;
						}
					}
				}
			}
			break;

			default:
				handleMessage(&header);
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
			/* TODO this may cause trouble with escape-codes. maybe we should store the
			 * "escape-state" somehow... */
			// do this just 3 times to ensure that we look for other events, too
			while(x < 3 && rbufPos < READ_BUF_SIZE &&
					(c = read(fd,rbuffer + rbufPos,READ_BUF_SIZE - rbufPos)) > 0) {
				*(rbuffer + rbufPos + c) = '\0';
				rbufPos += c;
				x++;
			}
			close(fd);

			// do we have enough data?
			if(rbufPos >= 256) {
				_sh->append(rbuffer);
				rbufPos = 0;
			}
		}
	}
}
