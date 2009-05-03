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
#include <esc/messages.h>
#include <esc/proc.h>
#include <esc/gui/window.h>
#include <esc/gui/uielement.h>
#include <esc/gui/color.h>
#include <esc/string.h>
#include <stdlib.h>

namespace esc {
	namespace gui {
		Color Window::BGCOLOR = Color(0x88,0x88,0x88);
		Color Window::TITLE_BGCOLOR = Color(0,0,0xFF);
		Color Window::TITLE_FGCOLOR = Color(0xFF,0xFF,0xFF);
		Color Window::BORDER_COLOR = Color(0x77,0x77,0x77);

		Window::Window(const String &title,tCoord x,tCoord y,tSize width,tSize height)
			: UIElement(x,y,width,height), _title(title), _titleBarHeight(20), _inTitle(false),
				_controls(Vector<Control*>()) {
			_g = new Graphics(x,y,width,height,Application::getInstance()->getColorDepth());

			// create window
			tFD winmgn = Application::getInstance()->getWinManagerFd();
			sMsgWinCreateReq msg;
			msg.header.id = MSG_WIN_CREATE_REQ;
			msg.header.length = sizeof(sMsgDataWinCreateReq);
			msg.data.x = x;
			msg.data.y = y;
			msg.data.width = width;
			msg.data.height = height;
			msg.data.owner = getpid();
			if(write(winmgn,&msg,sizeof(sMsgWinCreateReq)) != sizeof(sMsgWinCreateReq)) {
				printe("Unable to announce window to window-manager");
				exit(EXIT_FAILURE);
			}

			// read response
			sMsgHeader header;
			sMsgDataWinCreateResp data;
			if(read(winmgn,&header,sizeof(header)) != sizeof(header) ||
					read(winmgn,&data,sizeof(data)) != sizeof(data)) {
				printe("Invalid response to window-announcement");
				exit(EXIT_FAILURE);
			}

			// store window-id and add window to app
			_id = data.id;
			Application::getInstance()->AddWindow(this);
		}

		Window::~Window() {

		}

		void Window::onMouseMoved(const MouseEvent &e) {
			// we store on release/pressed wether we are in the header because
			// the delay between window-movement and cursor-movement may be too
			// big so that we "loose" the window
			if(_inTitle) {
				if(e.isButton1Down())
					move(e.getXMovement(),e.getYMovement());
				return;
			}
			passToCtrl(e,MOUSE_MOVED);
		}
		void Window::onMouseReleased(const MouseEvent &e) {
			if(e.getY() < _titleBarHeight)
				_inTitle = false;
			else
				passToCtrl(e,MOUSE_RELEASED);
		}
		void Window::onMousePressed(const MouseEvent &e) {
			if(e.getY() < _titleBarHeight)
				_inTitle = true;
			else
				passToCtrl(e,MOUSE_PRESSED);
		}
		void Window::onKeyPressed(const KeyEvent &e) {
			passToCtrl(e,KEY_PRESSED);
		}
		void Window::onKeyReleased(const KeyEvent &e) {
			passToCtrl(e,KEY_RELEASED);
		}

		void Window::passToCtrl(const KeyEvent &e,u8 event) {
			// TODO use focused control
			if(_controls.size() > 0) {
				switch(event) {
					case KEY_PRESSED:
						_controls[0]->onKeyPressed(e);
						break;
					case KEY_RELEASED:
						_controls[0]->onKeyReleased(e);
						break;
				}
			}
		}

		void Window::passToCtrl(const MouseEvent &e,u8 event) {
			Control *c;
			tCoord x = e.getX();
			tCoord y = e.getY();
			y -= _titleBarHeight;
			for(u32 i = 0; i < _controls.size(); i++) {
				c = _controls[i];
				if(x >= c->getX() && x < c->getX() + c->getWidth() &&
					y >= c->getY() && y < c->getY() + c->getHeight()) {
					switch(event) {
						case MOUSE_MOVED:
							c->onMouseMoved(e);
							break;
						case MOUSE_RELEASED:
							c->onMouseReleased(e);
							break;
						case MOUSE_PRESSED:
							c->onMousePressed(e);
							break;
					}
					break;
				}
			}
		}

		void Window::move(s16 x,s16 y) {
			tSize screenWidth = Application::getInstance()->getScreenWidth();
			tSize screenHeight = Application::getInstance()->getScreenHeight();
			if(getX() + x < 0)
				x = 0;
			else if(getX() + x + getWidth() >= screenWidth)
				x = screenWidth - getWidth();
			else
				x += getX();
			if(getY() + y < 0)
				y = 0;
			else if(getY() + y + getHeight() >= screenHeight)
				y = screenHeight - getHeight();
			else
				y += getY();
			moveTo(x,y);
		}

		void Window::moveTo(tCoord x,tCoord y) {
			if(getX() != x || getY() != y) {
				_g->move(x,y);
				setX(_g->_x);
				setY(_g->_y);

				sMsgWinMoveReq move;
				move.header.id = MSG_WIN_MOVE_REQ;
				move.header.length = sizeof(sMsgDataWinMoveReq);
				move.data.window = _id;
				move.data.x = getX();
				move.data.y = getY();
				write(Application::getInstance()->getWinManagerFd(),&move,sizeof(move));
			}
		}

		void Window::paint() {
			_g->setColor(BGCOLOR);
			_g->fillRect(0,_titleBarHeight,getWidth(),getHeight());
			_g->setColor(TITLE_BGCOLOR);
			_g->fillRect(0,0,getWidth(),_titleBarHeight);
			_g->setColor(TITLE_FGCOLOR);
			_g->drawString(5,(_titleBarHeight - _g->getFont().getHeight()) / 2,_title);
			_g->setColor(BORDER_COLOR);
			_g->drawLine(0,_titleBarHeight,getWidth(),_titleBarHeight);
			_g->drawRect(0,0,getWidth(),getHeight());
			for(u32 i = 0; i < _controls.size(); i++)
				_controls[i]->paint();
			_g->update();
		}

		void Window::add(Control &c) {
			_controls.add(&c);
			c.setWindow(this);
		}
	}
}
