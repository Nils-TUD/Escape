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
#include <esc/fileio.h>
#include <esc/messages.h>
#include <esc/mem.h>
#include <esc/stream.h>
#include <esc/gui/application.h>
#include <esc/gui/window.h>
#include <stdlib.h>

namespace esc {
	namespace gui {
		Application *Application::_inst = NULL;

		Application::Application() {
			_winFd = open("services:/winmanager",IO_READ | IO_WRITE);
			if(_winFd < 0) {
				printe("Unable to open window-manager");
				exit(EXIT_FAILURE);
			}

			_vesaFd = open("services:/vesa",IO_READ);
			if(_vesaFd < 0) {
				printe("Unable to open vesa");
				exit(EXIT_FAILURE);
			}

			_vesaMem = joinSharedMem("vesa");
			if(_vesaMem == NULL) {
				printe("Unable to open shared memory");
				exit(EXIT_FAILURE);
			}
		}

		Application::~Application() {
			close(_vesaFd);
			close(_winFd);
		}

		int Application::run() {
			sMsgHeader header;

			/* first, paint all windows */
			for(u32 i = 0; i < _windows.size(); i++)
				_windows[i]->paint();

			while(true) {
				if(read(_winFd,&header,sizeof(header)) != sizeof(header)) {
					printe("Read from window-manager failed");
					return EXIT_FAILURE;
				}

				switch(header.id) {
					case MSG_WIN_MOUSE: {
						sMsgDataWinMouse data;
						if(read(_winFd,&data,sizeof(data)) == sizeof(data)) {
							if(data.buttons) {
								for(u32 i = 0; i < _windows.size(); i++) {
									if(_windows[i]->getId() == data.window) {
										_windows[i]->move(data.movedX,data.movedY);
										break;
									}
								}
							}
						}
					}
					break;

					case MSG_WIN_REPAINT: {
						sMsgDataWinRepaint data;
						if(read(_winFd,&data,sizeof(data)) == sizeof(data)) {
							/*out.format("REPAINT @ %d,%d : %d,%d\n",
									data.x,data.y,data.width,data.height);*/
							for(u32 i = 0; i < _windows.size(); i++) {
								if(_windows[i]->getId() == data.window) {
									_windows[i]->update(data.x,data.y,data.width,data.height);
									break;
								}
							}
						}
					}
					break;
				}
			}

			return EXIT_SUCCESS;
		}
	}
}
