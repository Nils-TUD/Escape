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
#include <esc/string.h>
#include <stdlib.h>

// TODO ask vesa
#define BIT_PER_PIXEL		24

namespace esc {
	namespace gui {
		Window::Window(const String &title,tCoord x,tCoord y,tSize width,tSize height)
			: UIElement(x,y,width,height), _title(title), _titleBarHeight(20),
				_controls(Vector<Control*>()) {
			_g = new Graphics(x,y,width,height,BIT_PER_PIXEL);

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

		void Window::move(s16 x,s16 y) {
			if(getX() + x < 0)
				x = 0;
			else if(getX() + x + getWidth() >= RESOLUTION_X)
				x = RESOLUTION_X - getWidth();
			else
				x += getX();
			if(getY() + y < 0)
				y = 0;
			else if(getY() + y + getHeight() >= RESOLUTION_Y)
				y = RESOLUTION_Y - getHeight();
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
				yield();
			}
		}

		void Window::paint() {
			_g->setColor(bgColor);
			_g->fillRect(0,_titleBarHeight,getWidth(),getHeight());
			_g->setColor(titleBgColor);
			_g->fillRect(0,0,getWidth(),_titleBarHeight);
			_g->setColor(titleFgColor);
			_g->drawString(5,(_titleBarHeight - _g->getFont().getHeight()) / 2,_title);
			_g->setColor(borderColor);
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
