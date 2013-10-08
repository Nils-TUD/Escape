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
#include <esc/driver/vesa.h>
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

static void sighandler(int) {
}

namespace gui {
	Application *Application::_inst = nullptr;

	Application::Application()
			: _winFd(-1), _msg(), _run(true), _mouseBtns(0), _vesaInfo(), _windows(), _created(),
			  _activated(), _destroyed(), _queuelock(), _listening(false), _defTheme(nullptr) {
		if(crtlocku(&_queuelock) < 0)
			throw app_error("Unable to create queue lock");

		msgid_t mid;
		_winFd = open("/dev/winmanager",IO_MSGS);
		if(_winFd < 0)
			throw app_error("Unable to open window-manager");

		{
			int fd = open("/dev/vesa",IO_MSGS);
			if(fd < 0)
				throw app_error("Unable to open vesa");

			// request screen infos from vesa
			if(send(fd,MSG_VESA_GETMODE,&_msg,sizeof(_msg.args)) < 0)
				throw app_error("Unable to send get-mode-request to vesa");
			if(IGNSIGS(receive(fd,&mid,&_msg,sizeof(_msg))) < 0 || mid != MSG_VESA_GETMODE_RESP ||
					_msg.data.arg1 != 0) {
				throw app_error("Unable to read the get-mode-response from vesa");
			}
			memcpy(&_vesaInfo,_msg.data.d,sizeof(sVESAInfo));
			close(fd);
		}

		if(signal(SIG_USR1,sighandler) == SIG_ERR)
			throw app_error("Unable to announce USR1 signal handler");

		// init default theme
		_defTheme.setColor(Theme::CTRL_BACKGROUND,Color(0x88,0x88,0x88));
		_defTheme.setColor(Theme::CTRL_FOREGROUND,Color(0xFF,0xFF,0xFF));
		_defTheme.setColor(Theme::CTRL_BORDER,Color(0x55,0x55,0x55));
		_defTheme.setColor(Theme::CTRL_LIGHTBORDER,Color(0x70,0x70,0x70));
		_defTheme.setColor(Theme::CTRL_DARKBORDER,Color(0x20,0x20,0x20));
		_defTheme.setColor(Theme::CTRL_LIGHTBACK,Color(0x80,0x80,0x80));
		_defTheme.setColor(Theme::CTRL_DARKBACK,Color(0x60,0x60,0x60));
		_defTheme.setColor(Theme::BTN_BACKGROUND,Color(0x70,0x70,0x70));
		_defTheme.setColor(Theme::BTN_FOREGROUND,Color(0xFF,0xFF,0xFF));
		_defTheme.setColor(Theme::SEL_BACKGROUND,Color(0x00,0x00,0xFF));
		_defTheme.setColor(Theme::SEL_FOREGROUND,Color(0xFF,0xFF,0xFF));
		_defTheme.setColor(Theme::TEXT_BACKGROUND,Color(0xFF,0xFF,0xFF));
		_defTheme.setColor(Theme::TEXT_FOREGROUND,Color(0x00,0x00,0x00));
		_defTheme.setColor(Theme::WIN_TITLE_ACT_BG,Color(0,0,0xFF));
		_defTheme.setColor(Theme::WIN_TITLE_ACT_FG,Color(0xFF,0xFF,0xFF));
		_defTheme.setColor(Theme::WIN_TITLE_INACT_BG,Color(0,0,0x80));
		_defTheme.setColor(Theme::WIN_TITLE_INACT_FG,Color(0xFF,0xFF,0xFF));
		_defTheme.setColor(Theme::WIN_BORDER,Color(0x55,0x55,0x55));
		_defTheme.setPadding(2);
		_defTheme.setTextPadding(4);

		// subscribe to window-events
		{
			_msg.args.arg1 = MSG_WIN_CREATE_EV;
			if(send(_winFd,MSG_WIN_ADDLISTENER,&_msg,sizeof(_msg.args)) < 0)
				throw app_error("Unable to announce create-listener");
			_msg.args.arg1 = MSG_WIN_DESTROY_EV;
			if(send(_winFd,MSG_WIN_ADDLISTENER,&_msg,sizeof(_msg.args)) < 0)
				throw app_error("Unable to announce destroy-listener");
			_msg.args.arg1 = MSG_WIN_ACTIVE_EV;
			if(send(_winFd,MSG_WIN_ADDLISTENER,&_msg,sizeof(_msg.args)) < 0)
				throw app_error("Unable to announce active-listener");
		}
	}

	Application::~Application() {
		while(_windows.size() > 0)
			removeWindow(*_windows.begin());
		close(_winFd);
	}

	void Application::exit() {
		_run = false;
	}

	int Application::run() {
		msgid_t mid;
		while(_run) {
			int res = receive(_winFd,&mid,&_msg,sizeof(_msg));
			if(res < 0 && res != -EINTR)
				throw app_error(string("Read from window-manager failed: ") + strerror(-res));
			if(res >= 0)
				handleMessage(mid,&_msg);
			handleQueue();
		}
		return EXIT_SUCCESS;
	}

	void Application::handleQueue() {
		locku(&_queuelock);
		while(!_queue.empty()) {
			std::Functor<void> *functor = _queue.front();
			unlocku(&_queuelock);
			(*functor)();
			locku(&_queuelock);
			_queue.erase(_queue.begin());
		}
		unlocku(&_queuelock);
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

			case MSG_WIN_RESIZE_RESP: {
				gwinid_t win = (gwinid_t)msg->args.arg1;
				Window *w = getWindowById(win);
				if(w)
					w->onResized();
			}
			break;

			case MSG_WIN_UPDATE_RESP: {
				gwinid_t win = (gwinid_t)msg->args.arg1;
				Window *w = getWindowById(win);
				if(w)
					w->onUpdated();
			}
			break;

			case MSG_WIN_MOUSE_EV: {
				gpos_t x = (gpos_t)msg->args.arg1;
				gpos_t y = (gpos_t)msg->args.arg2;
				short movedX = (short)msg->args.arg3;
				short movedY = (short)msg->args.arg4;
				short movedZ = (short)msg->args.arg5;
				uchar buttons = (uchar)msg->args.arg6;
				gwinid_t win = (gwinid_t)msg->args.arg7;
				passToWindow(win,Pos(x,y),movedX,movedY,movedZ,buttons);
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

			case MSG_WIN_SET_ACTIVE_EV: {
				gwinid_t win = (gwinid_t)msg->args.arg1;
				bool isActive = (bool)msg->args.arg2;
				gpos_t mouseX = (gpos_t)msg->args.arg3;
				gpos_t mouseY = (gpos_t)msg->args.arg4;
				Window *w = getWindowById(win);
				if(w) {
					w->updateActive(isActive);
					if(isActive)
						closePopups(w->getId(),Pos(mouseX,mouseY));
				}
			}
			break;

			case MSG_WIN_CREATE_EV: {
				gwinid_t win = (gwinid_t)msg->str.arg1;
				string title(msg->str.s1);
				_created.send(win,title);
			}
			break;

			case MSG_WIN_ACTIVE_EV: {
				gwinid_t win = (gwinid_t)msg->args.arg1;
				_activated.send(win);
			}
			break;

			case MSG_WIN_DESTROY_EV: {
				gwinid_t win = (gwinid_t)msg->args.arg1;
				_destroyed.send(win);
			}
			break;
		}
	}

	void Application::passToWindow(gwinid_t win,const Pos &pos,short movedX,short movedY,
			short movedZ,uchar buttons) {
		bool moved,released,pressed;

		moved = movedX || movedY;
		released = _mouseBtns && !buttons;
		pressed = !_mouseBtns && buttons;
		_mouseBtns = buttons;

		Window *w = getWindowById(win);
		if(w) {
			Pos npos(max(0,min((gpos_t)_vesaInfo.width - 1,pos.x - w->getPos().x)),
			         max(0,min((gpos_t)_vesaInfo.height - 1,pos.y - w->getPos().y)));
			if(released) {
				MouseEvent event(MouseEvent::MOUSE_RELEASED,movedX,movedY,movedZ,npos,_mouseBtns);
				w->onMouseReleased(event);
			}
			else if(pressed) {
				MouseEvent event(MouseEvent::MOUSE_PRESSED,movedX,movedY,movedZ,npos,_mouseBtns);
				w->onMousePressed(event);
			}
			else if(moved) {
				MouseEvent event(MouseEvent::MOUSE_MOVED,movedX,movedY,movedZ,npos,_mouseBtns);
				w->onMouseMoved(event);
			}
			else if(movedZ) {
				MouseEvent event(MouseEvent::MOUSE_WHEEL,movedX,movedY,movedZ,npos,_mouseBtns);
				w->onMouseWheel(event);
			}
		}
	}

	void Application::closePopups(gwinid_t id,const Pos &pos) {
		for(auto it = _windows.begin(); it != _windows.end(); ++it) {
			if((*it)->getId() != id && (*it)->getStyle() == Window::POPUP) {
				shared_ptr<PopupWindow> pw = static_pointer_cast<PopupWindow>(*it);
				pw->close(pos);
				break;
			}
		}
	}

	void Application::requestWinUpdate(gwinid_t id,const Pos &pos,const Size &size) {
		Window *w = getWindowById(id);
		Size wsize = w->getSize();
		Size rsize = size;
		Pos rpos = pos;
		if(rpos.x < 0)
			rpos.x = 0;
		if(rpos.y < 0)
			rpos.y = 0;
		if(rpos.x + rsize.width > wsize.width)
			rsize.width = wsize.width - rpos.x;
		if(rpos.y + rsize.height > wsize.height)
			rsize.height = wsize.height - rpos.y;
		_msg.args.arg1 = id;
		_msg.args.arg2 = rpos.x;
		_msg.args.arg3 = rpos.y;
		_msg.args.arg4 = rsize.width;
		_msg.args.arg5 = rsize.height;
		if(send(_winFd,MSG_WIN_UPDATE,&_msg,sizeof(_msg.args)) < 0)
			throw app_error("Unable to request win-update");
	}

	void Application::requestActiveWindow(gwinid_t wid) {
		_msg.args.arg1 = wid;
		if(send(_winFd,MSG_WIN_SET_ACTIVE,&_msg,sizeof(_msg.args)) < 0)
			throw app_error("Unable to set window to active");
	}

	void Application::addWindow(shared_ptr<Window> win) {
		_windows.push_back(win);

		_msg.str.arg1 = win->getPos().x;
		_msg.str.arg2 = win->getPos().y;
		_msg.str.arg3 = win->getSize().width;
		_msg.str.arg4 = win->getSize().height;
		_msg.str.arg5 = win->getId();
		_msg.str.arg6 = win->getStyle();
		_msg.str.arg7 = win->getTitleBarHeight();
		if(win->hasTitleBar())
			strnzcpy(_msg.str.s1,win->getTitle().c_str(),sizeof(_msg.str.s1));
		else
			_msg.str.s1[0] = '\0';
		if(send(_winFd,MSG_WIN_CREATE,&_msg,sizeof(_msg.str)) < 0) {
			_windows.erase_first(win);
			throw app_error("Unable to announce window to window-manager");
		}
	}

	void Application::removeWindow(shared_ptr<Window> win) {
		if(_windows.erase_first(win)) {
			// let window-manager destroy our window
			_msg.args.arg1 = win->getId();
			if(send(_winFd,MSG_WIN_DESTROY,&_msg,sizeof(_msg.args)) < 0)
				throw app_error("Unable to destroy window");
		}
	}

	void Application::moveWindow(Window *win,bool finish) {
		_msg.args.arg1 = win->getId();
		_msg.args.arg2 = win->getMovePos().x;
		_msg.args.arg3 = win->getMovePos().y;
		_msg.args.arg4 = finish;
		if(send(_winFd,MSG_WIN_MOVE,&_msg,sizeof(_msg.args)) < 0)
			throw app_error("Unable to move window");
	}

	void Application::resizeWindow(Window *win,bool finish) {
		_msg.args.arg1 = win->getId();
		_msg.args.arg2 = win->getMovePos().x;
		_msg.args.arg3 = win->getMovePos().y;
		_msg.args.arg4 = win->getResizeSize().width;
		_msg.args.arg5 = win->getResizeSize().height;
		_msg.args.arg6 = finish;
		if(send(_winFd,MSG_WIN_RESIZE,&_msg,sizeof(_msg.args)) < 0)
			throw app_error("Unable to resize window");
	}

	Window *Application::getWindowById(gwinid_t id) {
		for(auto it = _windows.begin(); it != _windows.end(); ++it) {
			if((*it)->getId() == id)
				return it->get();
		}
		return nullptr;
	}
}
