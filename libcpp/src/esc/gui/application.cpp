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

		Application::Application()
				: _mouseBtns(0) {
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

			// request screen infos from vesa
			sMsgHeader header;
			header.id = MSG_VESA_GETMODE_REQ;
			header.length = 0;
			if(write(_vesaFd,&header,sizeof(header)) != sizeof(header)) {
				printe("Unable to send get-mode-request to vesa");
				exit(EXIT_FAILURE);
			}

			// read response
			sMsgDataVesaGetModeResp resp;
			if(read(_vesaFd,&header,sizeof(header)) != sizeof(header) ||
					read(_vesaFd,&resp,sizeof(resp)) != sizeof(resp)) {
				printe("Unable to read the get-mode-response from vesa");
				exit(EXIT_FAILURE);
			}

			// store it
			_screenWidth = resp.width;
			_screenHeight = resp.height;
			_colorDepth = resp.colorDepth;
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
						if(read(_winFd,&data,sizeof(data)) == sizeof(data))
							passToWindow(&data);
					}
					break;

					case MSG_WIN_REPAINT: {
						sMsgDataWinRepaint data;
						if(read(_winFd,&data,sizeof(data)) == sizeof(data)) {
							Window *w = getWindowById(data.window);
							if(w)
								w->update(data.x,data.y,data.width,data.height);
						}
					}
					break;
				}
			}

			return EXIT_SUCCESS;
		}

		void Application::passToWindow(sMsgDataWinMouse *e) {
			bool moved,released,pressed;

			moved = e->movedX || e->movedY;
			// TODO this is not correct
			released = _mouseBtns && !e->buttons;
			pressed = !_mouseBtns && e->buttons;
			_mouseBtns = e->buttons;

			Window *w = getWindowById(e->window);
			if(w) {
				tCoord x = MAX(0,MIN(_screenWidth - 1,e->x - w->getX()));
				tCoord y = MAX(0,MIN(_screenHeight - 1,e->y - w->getY()));
				MouseEvent event(e->movedX,e->movedY,x,y,_mouseBtns);
				if(released)
					w->onMouseReleased(event);
				else if(pressed)
					w->onMousePressed(event);
				else if(moved)
					w->onMouseMoved(event);
			}
		}

		Window *Application::getWindowById(tWinId id) {
			Window *w;
			for(u32 i = 0; i < _windows.size(); i++) {
				w = _windows[i];
				if(w->getId() == id)
					return w;
			}
			return NULL;
		}
	}
}
