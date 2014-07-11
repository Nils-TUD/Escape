/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <gui/window.h>
#include <gui/popupwindow.h>
#include <esc/messages.h>
#include <esc/esccodes.h>
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

static void sighandler(int sig) {
	// re-announce handler
	signal(sig,sighandler);
}

namespace gui {
	Application *Application::_inst = nullptr;

	const char *Application::getWinMng(const char *winmng) {
		if(winmng == NULL)
			winmng = getenv("WINMNG");
		if(winmng == NULL)
			throw app_error("Env-var WINMNG not set");
		return winmng;
	}

	Application::Application(const char *winmng)
			: _winMngName(getWinMng(winmng)), _winMng(_winMngName),
			  _winEv((std::string(_winMngName) + "-events").c_str()), _run(true), _mouseBtns(0),
			  _screenMode(_winMng.getMode()), _windows(), _created(), _activated(), _destroyed(),
			  _timequeue(), _queueMutex(), _listening(false), _defTheme(nullptr) {
		// announce signal handlers
		if(signal(SIGUSR1,sighandler) == SIG_ERR)
			throw app_error("Unable to announce USR1 signal handler");
		if(signal(SIGALRM,sighandler) == SIG_ERR)
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
		_defTheme.setColor(Theme::SEL_BACKGROUND,Color(0x11,0x44,0x94));
		_defTheme.setColor(Theme::SEL_FOREGROUND,Color(0xFF,0xFF,0xFF));
		_defTheme.setColor(Theme::TEXT_BACKGROUND,Color(0xFF,0xFF,0xFF));
		_defTheme.setColor(Theme::TEXT_FOREGROUND,Color(0x00,0x00,0x00));
		_defTheme.setColor(Theme::WIN_TITLE_ACT_BG,Color(0x11,0x44,0x94));
		_defTheme.setColor(Theme::WIN_TITLE_ACT_FG,Color(0xFF,0xFF,0xFF));
		_defTheme.setColor(Theme::WIN_TITLE_INACT_BG,Color(0x3d,0x4b,0x60));
		_defTheme.setColor(Theme::WIN_TITLE_INACT_FG,Color(0xFF,0xFF,0xFF));
		_defTheme.setColor(Theme::WIN_BORDER,Color(0x55,0x55,0x55));
		_defTheme.setPadding(2);
		_defTheme.setTextPadding(4);

		// subscribe to window-events
		_winEv.subscribe(ipc::WinMngEvents::Event::TYPE_CREATED);
		_winEv.subscribe(ipc::WinMngEvents::Event::TYPE_ACTIVE);
		_winEv.subscribe(ipc::WinMngEvents::Event::TYPE_DESTROYED);
	}

	Application::~Application() {
		while(_windows.size() > 0)
			removeWindow(*_windows.begin());
	}

	void Application::executeIn(uint msecs,std::Functor<void> *functor) {
		std::lock_guard<std::mutex> guard(_queueMutex);
		uint64_t tsc = rdtsc() + (msecs ? timetotsc(msecs * 1000) : 0);
		std::list<TimeoutFunctor>::iterator it;
		for(it = _timequeue.begin(); it != _timequeue.end(); ++it) {
			if(it->tsc > tsc)
				break;
		}
		if(msecs == 0)
			kill(getpid(),SIGUSR1);
		else if(it == _timequeue.begin())
			alarm(msecs);
		_timequeue.insert(it,TimeoutFunctor(tsc,functor));
	}

	void Application::exit() {
		_run = false;
	}

	int Application::run() {
		while(_run) {
			ipc::WinMngEvents::Event ev;
			ssize_t res = receive(_winEv.fd(),NULL,&ev,sizeof(ev));
			if(res < 0 && res != -EINTR)
				VTHROWE("receive from event-channel",res);
			if(res >= 0)
				handleEvent(ev);
			handleQueue();
		}
		return EXIT_SUCCESS;
	}

	void Application::handleQueue() {
		_queueMutex.lock();
		uint64_t next = std::numeric_limits<uint64_t>::max();
		// repeat until we've seen no calls anymore. this is because we release the lock in between
		// so that one can insert an item into the queue. if this is a callback with timeout 0, we
		// might have already got the signal and thus should handle it immediately.
		uint calls;
		do {
			calls = 0;
			for(auto it = _timequeue.begin(); it != _timequeue.end(); ) {
				auto cur = it++;
				if(cur->tsc <= rdtsc()) {
					_queueMutex.unlock();
					(*(cur->functor))();
					_queueMutex.lock();
					_timequeue.erase(cur);
					calls++;
				}
				else if(cur->tsc < next)
					next = cur->tsc;
			}
		}
		while(calls > 0);

		// program new wakeup
		uint64_t now = rdtsc();
		if(next > now && next != std::numeric_limits<uint64_t>::max())
			alarm(tsctotime(next - now) / 1000);
		_queueMutex.unlock();
	}

	void Application::handleEvent(const ipc::WinMngEvents::Event &ev) {
		switch(ev.type) {
			case ipc::WinMngEvents::Event::TYPE_RESIZE: {
				Window *w = getWindowById(ev.wid);
				if(w)
					w->onResized();
			}
			break;

			case ipc::WinMngEvents::Event::TYPE_MOUSE:
				passToWindow(ev.wid,Pos(ev.d.mouse.x,ev.d.mouse.y),
					ev.d.mouse.movedX,ev.d.mouse.movedY,ev.d.mouse.movedZ,ev.d.mouse.buttons);
				break;

			case ipc::WinMngEvents::Event::TYPE_KEYBOARD: {
				Window *w = getWindowById(ev.wid);
				if(w) {
					if(ev.d.keyb.modifier & STATE_BREAK) {
						KeyEvent e(KeyEvent::KEY_RELEASED,
							ev.d.keyb.keycode,ev.d.keyb.character,ev.d.keyb.modifier);
						w->onKeyReleased(e);
					}
					else {
						KeyEvent e(KeyEvent::KEY_PRESSED,
							ev.d.keyb.keycode,ev.d.keyb.character,ev.d.keyb.modifier);
						w->onKeyPressed(e);
					}
				}
			}
			break;

			case ipc::WinMngEvents::Event::TYPE_SET_ACTIVE: {
				Window *w = getWindowById(ev.wid);
				if(w) {
					w->updateActive(ev.d.setactive.active);
					if(ev.d.setactive.active)
						closePopups(w->getId(),Pos(ev.d.setactive.mouseX,ev.d.setactive.mouseY));
				}
			}
			break;

			case ipc::WinMngEvents::Event::TYPE_CREATED:
				_created.send(ev.wid,ev.d.created.title);
				break;

			case ipc::WinMngEvents::Event::TYPE_ACTIVE:
				_activated.send(ev.wid);
				break;

			case ipc::WinMngEvents::Event::TYPE_DESTROYED:
				_destroyed.send(ev.wid);
				break;

			case ipc::WinMngEvents::Event::TYPE_RESET: {
				Window *w = getWindowById(ev.wid);
				// re-request screen infos from vesa
				_screenMode = _winMng.getMode();
				if(w)
					w->onReset();
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
		_winMng.update(id,rpos.x,rpos.y,rsize.width,rsize.height);
	}

	void Application::addWindow(shared_ptr<Window> win) {
		gwinid_t id = _winMng.create(win->getPos().x,win->getPos().y,
			win->getSize().width,win->getSize().height,win->getStyle(),
			win->getTitleBarHeight(),win->hasTitleBar() ? win->getTitle().c_str() : "");
		_windows.push_back(win);
		win->onCreated(id);
		_winEv.attach(win->getId());
	}

	void Application::removeWindow(shared_ptr<Window> win) {
		if(_windows.erase_first(win)) {
			// let window-manager destroy our window
			_winMng.destroy(win->getId());
		}
	}

	void Application::moveWindow(Window *win,bool finish) {
		_winMng.move(win->getId(),win->getMovePos().x,win->getMovePos().y,finish);
	}

	void Application::resizeWindow(Window *win,bool finish) {
		_winMng.resize(win->getId(),win->getMovePos().x,win->getMovePos().y,
			win->getResizeSize().width,win->getResizeSize().height,finish);
	}

	Window *Application::getWindowById(gwinid_t id) {
		for(auto it = _windows.begin(); it != _windows.end(); ++it) {
			if((*it)->getId() == id)
				return it->get();
		}
		return nullptr;
	}
}
