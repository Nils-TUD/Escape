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
#include <esc/driver/screen.h>
#include <gui/application.h>
#include <gui/window.h>
#include <gui/popupwindow.h>
#include <esc/messages.h>
#include <esc/thread.h>
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/mem.h>
#include <esc/time.h>
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

static void sighandler(int) {
}

namespace gui {
	Application *Application::_inst = nullptr;

	Application::Application(const char *winmng)
			: _winFd(-1), _winEvFd(-1), _run(true), _mouseBtns(0), _screenMode(), _windows(), _created(),
			  _activated(), _destroyed(), _timequeue(), _queuelock(), _listening(false),
			  _defTheme(nullptr), _winmng() {
		if(crtlocku(&_queuelock) < 0)
			throw app_error("Unable to create queue lock");

		if(winmng == NULL)
			winmng = getenv("WINMNG");
		if(winmng == NULL)
			throw app_error("Env-var WINMNG not set");
		_winmng = winmng;

		_winFd = open(winmng,IO_MSGS);
		if(_winFd < 0)
			throw app_error("Unable to open window-manager");

		char path[MAX_PATH_LEN];
		snprintf(path,sizeof(path),"%s-events",winmng);
		_winEvFd = open(path,IO_MSGS);
		if(_winEvFd < 0)
			throw app_error("Unable to open window-manager's event channel");

		// request screen infos from vesa
		if(screen_getMode(_winFd,&_screenMode) < 0)
			throw app_error("Unable to get mode");

		if(signal(SIG_USR1,sighandler) == SIG_ERR)
			throw app_error("Unable to announce USR1 signal handler");
		if(signal(SIG_ALARM,sigalarm) == SIG_ERR)
			throw app_error("Unable to announce ALARM signal handler");

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
			sArgsMsg msg;
			msg.arg1 = MSG_WIN_CREATE_EV;
			if(send(_winEvFd,MSG_WIN_ADDLISTENER,&msg,sizeof(msg)) < 0)
				throw app_error("Unable to announce create-listener");
			msg.arg1 = MSG_WIN_DESTROY_EV;
			if(send(_winEvFd,MSG_WIN_ADDLISTENER,&msg,sizeof(msg)) < 0)
				throw app_error("Unable to announce destroy-listener");
			msg.arg1 = MSG_WIN_ACTIVE_EV;
			if(send(_winEvFd,MSG_WIN_ADDLISTENER,&msg,sizeof(msg)) < 0)
				throw app_error("Unable to announce active-listener");
		}
	}

	Application::~Application() {
		while(_windows.size() > 0)
			removeWindow(*_windows.begin());
		close(_winEvFd);
		close(_winFd);
	}

	void Application::executeIn(uint msecs,std::Functor<void> *functor) {
		locku(&_queuelock);
		uint64_t tsc = rdtsc() + timetotsc(msecs * 1000);
		std::list<TimeoutFunctor>::iterator it;
		for(it = _timequeue.begin(); it != _timequeue.end(); ++it) {
			if(it->tsc > tsc)
				break;
		}
		if(msecs == 0)
			kill(getpid(),SIG_USR1);
		else if(it == _timequeue.begin())
			alarm(msecs);
		_timequeue.insert(it,TimeoutFunctor(tsc,functor));
		unlocku(&_queuelock);
	}

	void Application::sigalarm(int) {
	}

	void Application::exit() {
		_run = false;
	}

	int Application::run() {
		msgid_t mid;
		while(_run) {
			sMsg msg;
			int res = receive(_winEvFd,&mid,&msg,sizeof(msg));
			if(res < 0 && res != -EINTR)
				throw app_error(string("Read from window-manager failed: ") + strerror(-res));
			if(res >= 0)
				handleMessage(mid,&msg);
			handleQueue();
		}
		return EXIT_SUCCESS;
	}

	void Application::handleQueue() {
		locku(&_queuelock);
		uint64_t now = rdtsc();
		for(auto it = _timequeue.begin(); it != _timequeue.end(); ) {
			auto cur = it++;
			if(cur->tsc <= now) {
				unlocku(&_queuelock);
				(*(cur->functor))();
				locku(&_queuelock);
				_timequeue.erase(cur);
			}
		}
		unlocku(&_queuelock);
	}

	void Application::handleMessage(msgid_t mid,sMsg *msg) {
		switch(mid) {
			case MSG_WIN_RESIZE_RESP: {
				gwinid_t win = (gwinid_t)msg->args.arg1;
				Window *w = getWindowById(win);
				if(w)
					w->onResized();
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
			Pos npos(max(0,min((gpos_t)_screenMode.width - 1,pos.x - w->getPos().x)),
			         max(0,min((gpos_t)_screenMode.height - 1,pos.y - w->getPos().y)));
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
		sArgsMsg msg;
		msg.arg1 = id;
		msg.arg2 = rpos.x;
		msg.arg3 = rpos.y;
		msg.arg4 = rsize.width;
		msg.arg5 = rsize.height;
		msgid_t mid = MSG_WIN_UPDATE;
		if(SENDRECV_IGNSIGS(_winFd,&mid,&msg,sizeof(msg)) < 0)
			throw app_error("Unable to request win-update");
	}

	void Application::requestActiveWindow(gwinid_t wid) {
		sArgsMsg msg;
		msg.arg1 = wid;
		if(send(_winFd,MSG_WIN_SET_ACTIVE,&msg,sizeof(msg)) < 0)
			throw app_error("Unable to set window to active");
	}

	void Application::addWindow(shared_ptr<Window> win) {
		_windows.push_back(win);

		sMsg msg;
		msg.str.arg1 = win->getPos().x;
		msg.str.arg2 = win->getPos().y;
		msg.str.arg3 = win->getSize().width;
		msg.str.arg4 = win->getSize().height;
		msg.str.arg5 = win->getStyle();
		msg.str.arg6 = win->getTitleBarHeight();
		if(win->hasTitleBar())
			strnzcpy(msg.str.s1,win->getTitle().c_str(),sizeof(msg.str.s1));
		else
			msg.str.s1[0] = '\0';
		msgid_t mid = MSG_WIN_CREATE;
		if(SENDRECV_IGNSIGS(_winFd,&mid,&msg,sizeof(msg)) < 0) {
			_windows.erase_first(win);
			throw app_error("Unable to announce window to window-manager");
		}
		win->onCreated(msg.args.arg1);

		msg.args.arg1 = win->getId();
		// arg2 does already contain the randId
		if(send(_winEvFd,MSG_WIN_ATTACH,&msg,sizeof(msg.args)) < 0)
			throw app_error("Unable to attach window to event-channel");
	}

	void Application::removeWindow(shared_ptr<Window> win) {
		if(_windows.erase_first(win)) {
			sArgsMsg msg;
			// let window-manager destroy our window
			msg.arg1 = win->getId();
			if(send(_winFd,MSG_WIN_DESTROY,&msg,sizeof(msg)) < 0)
				throw app_error("Unable to destroy window");
		}
	}

	void Application::moveWindow(Window *win,bool finish) {
		sArgsMsg msg;
		msg.arg1 = win->getId();
		msg.arg2 = win->getMovePos().x;
		msg.arg3 = win->getMovePos().y;
		msg.arg4 = finish;
		if(send(_winFd,MSG_WIN_MOVE,&msg,sizeof(msg)) < 0)
			throw app_error("Unable to move window");
	}

	void Application::resizeWindow(Window *win,bool finish) {
		sArgsMsg msg;
		msg.arg1 = win->getId();
		msg.arg2 = win->getMovePos().x;
		msg.arg3 = win->getMovePos().y;
		msg.arg4 = win->getResizeSize().width;
		msg.arg5 = win->getResizeSize().height;
		msg.arg6 = finish;
		if(send(_winFd,MSG_WIN_RESIZE,&msg,sizeof(msg)) < 0)
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
