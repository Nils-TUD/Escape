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

#include <gui/popupwindow.h>
#include <gui/window.h>
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/esccodes.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/time.h>
#include <algorithm>
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
			  _screenMode(_winMng.getMode()), _windows(),
			  _created(), _activated(), _destroyed(),
			  _timequeue(), _queueMutex(), _listening(false),
			  _themeName(_winMng.getTheme()), _baseTheme(loadTheme()) {
		// announce signal handlers
		if(signal(SIGUSR1,sighandler) == SIG_ERR)
			throw app_error("Unable to announce USR1 signal handler");
		if(signal(SIGALRM,sighandler) == SIG_ERR)
			throw app_error("Unable to announce ALARM signal handler");

		// subscribe to window-events
		_winEv.subscribe(esc::WinMngEvents::Event::TYPE_CREATED);
		_winEv.subscribe(esc::WinMngEvents::Event::TYPE_ACTIVE);
		_winEv.subscribe(esc::WinMngEvents::Event::TYPE_DESTROYED);
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
			ualarm(msecs * 1000);
		_timequeue.insert(it,TimeoutFunctor(tsc,functor));
	}

	void Application::exit() {
		_run = false;
	}

	int Application::run() {
		while(_run) {
			esc::WinMngEvents::Event ev;
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
		// repeat until we've seen no calls anymore. this is because we release the lock in between
		// so that one can insert an item into the queue. if this is a callback with timeout 0, we
		// might have already got the signal and thus should handle it immediately.
		uint64_t now = rdtsc();
		while(!_timequeue.empty()) {
			auto cur = _timequeue.front();
			if(cur.tsc > (now = rdtsc()))
				break;

			_timequeue.pop_front();
			_queueMutex.unlock();
			(*(cur.functor))();
			_queueMutex.lock();
		}

		// program new wakeup
		if(!_timequeue.empty()) {
			auto first = _timequeue.front();
			ualarm(tsctotime(first.tsc - now));
		}
		_queueMutex.unlock();
	}

	void Application::handleEvent(const esc::WinMngEvents::Event &ev) {
		switch(ev.type) {
			case esc::WinMngEvents::Event::TYPE_RESIZE: {
				Window *w = getWindowById(ev.wid);
				if(w)
					w->onResized();
			}
			break;

			case esc::WinMngEvents::Event::TYPE_MOUSE:
				passToWindow(ev.wid,Pos(ev.d.mouse.x,ev.d.mouse.y),
					ev.d.mouse.movedX,ev.d.mouse.movedY,ev.d.mouse.movedZ,ev.d.mouse.buttons);
				break;

			case esc::WinMngEvents::Event::TYPE_KEYBOARD: {
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

			case esc::WinMngEvents::Event::TYPE_SET_ACTIVE: {
				Window *w = getWindowById(ev.wid);
				if(w) {
					w->updateActive(ev.d.setactive.active);
					if(ev.d.setactive.active)
						closePopups(w->getId(),Pos(ev.d.setactive.mouseX,ev.d.setactive.mouseY));
				}
			}
			break;

			case esc::WinMngEvents::Event::TYPE_CREATED:
				_created.send(ev.wid,ev.d.created.title);
				break;

			case esc::WinMngEvents::Event::TYPE_ACTIVE:
				_activated.send(ev.wid);
				break;

			case esc::WinMngEvents::Event::TYPE_DESTROYED:
				_destroyed.send(ev.wid);
				break;

			case esc::WinMngEvents::Event::TYPE_RESET: {
				// update theme, if it has changed
				std::string theme = _winMng.getTheme();
				if(theme != _themeName) {
					_themeName = theme;
					_baseTheme = loadTheme();
				}

				// re-request screen infos from vesa
				_screenMode = _winMng.getMode();

				// reset window
				Window *w = getWindowById(ev.wid);
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

	void Application::shareWinBuf(Window *win,int fd) {
		_winMng.shareWinBuf(win->getId(),fd);
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

	BaseTheme Application::loadTheme() {
		esc::FStream themefile(("/etc/themes/" + getTheme()).c_str(),"r");
		return BaseTheme::unserialize(themefile);
	}
}
