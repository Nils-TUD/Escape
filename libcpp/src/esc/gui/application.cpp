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
#include <messages.h>
#include <esc/mem.h>
#include <esc/thread.h>
#include <esc/stream.h>
#include <esc/gui/application.h>
#include <esc/gui/window.h>
#include <esc/gui/popupwindow.h>
#include <stdlib.h>

namespace esc {
	namespace gui {
		Application *Application::_inst = NULL;

		Application::Application()
				: _winFd(-1), _mouseBtns(0), _vesaFd(-1), _vesaMem(NULL),
				  _screenWidth(0), _screenHeight(0), _colorDepth(0), _windows(Vector<Window*>()) {
			tMsgId mid;
			_winFd = open("/services/winmanager",IO_READ | IO_WRITE);
			if(_winFd < 0) {
				printe("Unable to open window-manager");
				exit(EXIT_FAILURE);
			}

			_vesaFd = open("/services/vesa",IO_READ);
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
			if(send(_vesaFd,MSG_VESA_GETMODE_REQ,&_msg,sizeof(_msg.args)) < 0) {
				printe("Unable to send get-mode-request to vesa");
				exit(EXIT_FAILURE);
			}
			if(receive(_vesaFd,&mid,&_msg,sizeof(_msg)) < 0 || mid != MSG_VESA_GETMODE_RESP) {
				printe("Unable to read the get-mode-response from vesa");
				exit(EXIT_FAILURE);
			}

			// store it
			_screenWidth = (tSize)_msg.args.arg1;
			_screenHeight = (tSize)_msg.args.arg2;
			_colorDepth = (tColDepth)_msg.args.arg3;
		}

		Application::~Application() {
			close(_vesaFd);
			close(_winFd);
		}

		int Application::run() {
			while(true)
				doEvents();
			return EXIT_SUCCESS;
		}

		void Application::doEvents() {
			tMsgId mid;
			if(receive(_winFd,&mid,&_msg,sizeof(_msg)) < 0) {
				printe("Read from window-manager failed");
				exit(EXIT_FAILURE);
			}
			handleMessage(mid,&_msg);
		}

		void Application::handleMessage(tMsgId mid,const sMsg *msg) {
			switch(mid) {
				case MSG_WIN_CREATE_RESP: {
					u16 tmpId = msg->args.arg1;
					u16 id = msg->args.arg2;
					// notify the window that it has been created
					Window *w = getWindowById(tmpId);
					if(w)
						w->onCreated(id);
				}
				break;

				case MSG_WIN_MOUSE: {
					u16 x = (u16)msg->args.arg1;
					u16 y = (u16)msg->args.arg2;
					s16 movedX = (s16)msg->args.arg3;
					s16 movedY = (s16)msg->args.arg4;
					u8 buttons = (u8)msg->args.arg5;
					tWinId win = (tWinId)msg->args.arg6;
					passToWindow(win,x,y,movedX,movedY,buttons);
				}
				break;

				case MSG_WIN_KEYBOARD: {
					u8 keycode = (u8)msg->args.arg1;
					bool isBreak = (bool)msg->args.arg2;
					tWinId win = (tWinId)msg->args.arg3;
					char character = (char)msg->args.arg4;
					u8 modifier = (u8)msg->args.arg5;
					Window *w = getWindowById(win);
					/*debugf("kc=%d, brk=%d, win=%d, char='%c' (%d), modifier=%x\n",keycode,isBreak,
							win,character,character,modifier);*/
					if(w) {
						if(isBreak) {
							KeyEvent e(KeyEvent::KEY_RELEASED,keycode,character,modifier);
							w->onKeyReleased(e);
						}
						else {
							KeyEvent e(KeyEvent::KEY_PRESSED,keycode,character,modifier);
							w->onKeyPressed(e);
						}
					}
				}
				break;

				case MSG_WIN_UPDATE: {
					tCoord x = (tCoord)msg->args.arg1;
					tCoord y = (tCoord)msg->args.arg2;
					tSize width = (tSize)msg->args.arg3;
					tSize height = (tSize)msg->args.arg4;
					tWinId win = (tWinId)msg->args.arg5;
					Window *w = getWindowById(win);
					if(w)
						w->update(x,y,width,height);
				}
				break;

				case MSG_WIN_SET_ACTIVE: {
					tWinId win = (tWinId)msg->args.arg1;
					bool isActive = (bool)msg->args.arg2;
					tCoord mouseX = (tCoord)msg->args.arg3;
					tCoord mouseY = (tCoord)msg->args.arg4;
					Window *w = getWindowById(win);
					if(w) {
						w->setActive(isActive);
						if(isActive)
							closePopups(w->getId(),mouseX,mouseY);
					}
				}
				break;
			}
		}

		void Application::passToWindow(tWinId win,u16 x,u16 y,s16 movedX,s16 movedY,u8 buttons) {
			bool moved,released,pressed;

			moved = movedX || movedY;
			// TODO this is not correct
			released = _mouseBtns && !buttons;
			pressed = !_mouseBtns && buttons;
			_mouseBtns = buttons;

			Window *w = getWindowById(win);
			if(w) {
				tCoord nx = MAX(0,MIN(_screenWidth - 1,x - w->getX()));
				tCoord ny = MAX(0,MIN(_screenHeight - 1,y - w->getY()));

				if(released) {
					MouseEvent event(MouseEvent::MOUSE_RELEASED,movedX,movedY,nx,ny,_mouseBtns);
					w->onMouseReleased(event);
				}
				else if(pressed) {
					MouseEvent event(MouseEvent::MOUSE_PRESSED,movedX,movedY,nx,ny,_mouseBtns);
					w->onMousePressed(event);
				}
				else if(moved) {
					MouseEvent event(MouseEvent::MOUSE_MOVED,movedX,movedY,nx,ny,_mouseBtns);
					w->onMouseMoved(event);
				}
			}
		}

		void Application::closePopups(tWinId id,tCoord x,tCoord y) {
			for(u32 i = 0; i < _windows.size(); i++) {
				Window *pw = _windows[i];
				if(pw->getId() != id && pw->getStyle() == Window::STYLE_POPUP) {
					((PopupWindow*)pw)->close(x,y);
					break;
				}
			}
		}

		void Application::requestWinUpdate(tWinId id,tCoord x,tCoord y,tSize width,tSize height) {
			_msg.args.arg1 = id;
			_msg.args.arg2 = x;
			_msg.args.arg3 = y;
			_msg.args.arg4 = width;
			_msg.args.arg5 = height;
			if(send(_winFd,MSG_WIN_UPDATE_REQ,&_msg,sizeof(_msg.args)) < 0) {
				printe("Unable to request win-update");
				exit(EXIT_FAILURE);
			}
		}

		void Application::addWindow(Window *win) {
			_windows.add(win);

			_msg.args.arg1 = (win->getX() << 16) | win->getY();
			_msg.args.arg2 = (win->getWidth() << 16) | win->getHeight();
			_msg.args.arg3 = win->getId();
			_msg.args.arg4 = gettid();
			_msg.args.arg5 = win->getStyle();
			if(send(_winFd,MSG_WIN_CREATE_REQ,&_msg,sizeof(_msg.args)) < 0) {
				printe("Unable to announce window to window-manager");
				exit(EXIT_FAILURE);
			}
		}

		void Application::removeWindow(Window *win) {
			_windows.removeFirst(win);

			// let window-manager destroy our window
			_msg.args.arg1 = win->getId();
			if(send(_winFd,MSG_WIN_DESTROY_REQ,&_msg,sizeof(_msg.args)) < 0) {
				printe("Unable to destroy window");
				exit(EXIT_FAILURE);
			}
		}

		void Application::moveWindow(Window *win) {
			_msg.args.arg1 = win->getId();
			_msg.args.arg2 = win->getX();
			_msg.args.arg3 = win->getY();
			if(send(_winFd,MSG_WIN_MOVE_REQ,&_msg,sizeof(_msg.args)) < 0) {
				printe("Unable to move window");
				exit(EXIT_FAILURE);
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
