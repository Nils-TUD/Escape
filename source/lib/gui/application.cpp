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
#include <gui/application.h>
#include <gui/window.h>
#include <gui/popupwindow.h>
#include <esc/messages.h>
#include <esc/thread.h>
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/mem.h>
#include <algorithm>
#include <iostream>
#include <stdio.h>

namespace gui {
	Application *Application::_inst = NULL;

	Application::Application()
			: _winFd(-1), _msg(sMsg()), _run(true), _mouseBtns(0), _vesaFd(-1), _vesaMem(NULL),
			  _vesaInfo(sVESAInfo()), _windows(vector<Window*>()) {
		msgid_t mid;
		_winFd = open("/dev/winmanager",IO_MSGS);
		if(_winFd < 0)
			throw app_error("Unable to open window-manager");

		_vesaFd = open("/dev/vesa",IO_MSGS);
		if(_vesaFd < 0)
			throw app_error("Unable to open vesa");

		_vesaMem = joinSharedMem("vesa");
		if(_vesaMem == NULL)
			throw app_error("Unable to open shared memory");

		// request screen infos from vesa
		if(send(_vesaFd,MSG_VESA_GETMODE,&_msg,sizeof(_msg.args)) < 0)
			throw app_error("Unable to send get-mode-request to vesa");
		if(RETRY(receive(_vesaFd,&mid,&_msg,sizeof(_msg))) < 0 || mid != MSG_VESA_GETMODE_RESP ||
				_msg.data.arg1 != 0) {
			throw app_error("Unable to read the get-mode-response from vesa");
		}

		// store it
		memcpy(&_vesaInfo,_msg.data.d,sizeof(sVESAInfo));
	}

	Application::~Application() {
		while(_windows.size() > 0)
			removeWindow(*_windows.begin());
		close(_vesaFd);
		close(_winFd);
	}

	void Application::exit() {
		_run = false;
	}

	int Application::run() {
		msgid_t mid;
		while(_run) {
			if(RETRY(receive(_winFd,&mid,&_msg,sizeof(_msg))) < 0)
				throw app_error("Read from window-manager failed");
			handleMessage(mid,&_msg);
		}
		return EXIT_SUCCESS;
	}

	void Application::handleMessage(msgid_t mid,sMsg *msg) {
		switch(mid) {
			case MSG_WIN_CREATE_RESP: {
				gwinid_t tmpId = msg->args.arg1;
				gwinid_t id = msg->args.arg2;
				// notify the window that it has been created
				Window *w = getWindowById(tmpId);
				if(w)
					w->onCreated(id);
			}
			break;

			case MSG_WIN_MOUSE_EV: {
				gpos_t x = (gpos_t)msg->args.arg1;
				gpos_t y = (gpos_t)msg->args.arg2;
				short movedX = (short)msg->args.arg3;
				short movedY = (short)msg->args.arg4;
				uchar buttons = (uchar)msg->args.arg5;
				gwinid_t win = (gwinid_t)msg->args.arg6;
				passToWindow(win,x,y,movedX,movedY,buttons);
			}
			break;

			case MSG_WIN_KEYBOARD_EV: {
				uchar keycode = (uchar)msg->args.arg1;
				bool isBreak = (bool)msg->args.arg2;
				gwinid_t win = (gwinid_t)msg->args.arg3;
				char character = (char)msg->args.arg4;
				uchar modifier = (uchar)msg->args.arg5;
				Window *w = getWindowById(win);
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

			case MSG_WIN_UPDATE_EV: {
				gpos_t x = (gpos_t)msg->args.arg1;
				gpos_t y = (gpos_t)msg->args.arg2;
				gsize_t width = (gsize_t)msg->args.arg3;
				gsize_t height = (gsize_t)msg->args.arg4;
				gwinid_t win = (gwinid_t)msg->args.arg5;
				Window *w = getWindowById(win);
				if(w) {
					if(x + width > w->getWidth())
						width = w->getWidth() - x;
					if(y + height > w->getHeight())
						height = w->getHeight() - y;
					w->update(x,y,width,height);
				}
			}
			break;

			case MSG_WIN_SET_ACTIVE: {
				gwinid_t win = (gwinid_t)msg->args.arg1;
				bool isActive = (bool)msg->args.arg2;
				gpos_t mouseX = (gpos_t)msg->args.arg3;
				gpos_t mouseY = (gpos_t)msg->args.arg4;
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

	void Application::passToWindow(gwinid_t win,gpos_t x,gpos_t y,short movedX,short movedY,
			uchar buttons) {
		bool moved,released,pressed;

		moved = movedX || movedY;
		released = _mouseBtns && !buttons;
		pressed = !_mouseBtns && buttons;
		_mouseBtns = buttons;

		Window *w = getWindowById(win);
		if(w) {
			gpos_t nx = MAX(0,MIN(_vesaInfo.width - 1,x - w->getX()));
			gpos_t ny = MAX(0,MIN(_vesaInfo.height - 1,y - w->getY()));

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

	void Application::closePopups(gwinid_t id,gpos_t x,gpos_t y) {
		for(vector<Window*>::iterator it = _windows.begin(); it != _windows.end(); ++it) {
			Window *pw = *it;
			if(pw->getId() != id && pw->getStyle() == Window::STYLE_POPUP) {
				((PopupWindow*)pw)->close(x,y);
				break;
			}
		}
	}

	void Application::requestWinUpdate(gwinid_t id,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		Window *w = getWindowById(id);
		if(x < 0)
			x = 0;
		if(y < 0)
			y = 0;
		if(x + width > w->getWidth())
			width = w->getWidth() - x;
		if(y + height > w->getHeight())
			height = w->getHeight() - y;
		_msg.args.arg1 = id;
		_msg.args.arg2 = x;
		_msg.args.arg3 = y;
		_msg.args.arg4 = width;
		_msg.args.arg5 = height;
		if(send(_winFd,MSG_WIN_UPDATE,&_msg,sizeof(_msg.args)) < 0)
			throw app_error("Unable to request win-update");
	}

	void Application::addWindow(Window *win) {
		_windows.push_back(win);

		_msg.args.arg1 = (win->getX() << 16) | win->getY();
		_msg.args.arg2 = (win->getWidth() << 16) | win->getHeight();
		_msg.args.arg3 = win->getId();
		_msg.args.arg4 = win->getStyle();
		_msg.args.arg5 = win->getTitleBarHeight();
		if(send(_winFd,MSG_WIN_CREATE,&_msg,sizeof(_msg.args)) < 0) {
			_windows.erase_first(win);
			throw app_error("Unable to announce window to window-manager");
		}
	}

	void Application::removeWindow(Window *win) {
		if(_windows.erase_first(win)) {
			// let window-manager destroy our window
			_msg.args.arg1 = win->getId();
			if(send(_winFd,MSG_WIN_DESTROY,&_msg,sizeof(_msg.args)) < 0)
				throw app_error("Unable to destroy window");
		}
	}

	void Application::moveWindow(Window *win,bool finish) {
		_msg.args.arg1 = win->getId();
		_msg.args.arg2 = win->getMoveX();
		_msg.args.arg3 = win->getMoveY();
		_msg.args.arg4 = finish;
		if(send(_winFd,MSG_WIN_MOVE,&_msg,sizeof(_msg.args)) < 0)
			throw app_error("Unable to move window");
	}

	void Application::resizeWindow(Window *win,bool finish) {
		_msg.args.arg1 = win->getId();
		_msg.args.arg2 = win->getMoveX();
		_msg.args.arg3 = win->getMoveY();
		_msg.args.arg4 = win->getResizeWidth();
		_msg.args.arg5 = win->getResizeHeight();
		_msg.args.arg6 = finish;
		if(send(_winFd,MSG_WIN_RESIZE,&_msg,sizeof(_msg.args)) < 0)
			throw app_error("Unable to resize window");
	}

	Window *Application::getWindowById(gwinid_t id) {
		for(vector<Window*>::iterator it = _windows.begin(); it != _windows.end(); ++it) {
			if((*it)->getId() == id)
				return *it;
		}
		return NULL;
	}
}
