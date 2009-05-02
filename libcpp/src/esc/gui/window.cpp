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
#include <stdlib.h>

// TODO ask vesa
#define BIT_PER_PIXEL		16

namespace esc {
	namespace gui {
		Window::Window(tCoord x,tCoord y,tSize width,tSize height)
			: _x(x), _y(y), _w(width), _h(height), _g(new Graphics(x,y,width,height,BIT_PER_PIXEL)) {
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
			delete _g;
		}

		void Window::move(s16 x,s16 y) {
			if((s32)_x + x < 0)
				x = 0;
			else if(_x + x + _w >= RESOLUTION_X)
				x = RESOLUTION_X - _w;
			else
				x += _x;
			if((s32)_y + y < 0)
				y = 0;
			else if(_y + y + _h >= RESOLUTION_Y)
				y = RESOLUTION_Y - _h;
			else
				y += _y;
			moveTo(x,y);
		}

		void Window::moveTo(tCoord x,tCoord y) {
			if(_x != x || _y != y) {
				_g->move(x,y);
				_x = _g->_x;
				_y = _g->_y;

				sMsgWinMoveReq move;
				move.header.id = MSG_WIN_MOVE_REQ;
				move.header.length = sizeof(sMsgDataWinMoveReq);
				move.data.window = _id;
				move.data.x = _x;
				move.data.y = _y;
				write(Application::getInstance()->getWinManagerFd(),&move,sizeof(move));
				yield();
			}
		}

		void Window::paint() {
			tColor colors[] = {0xF00F,0x00FF,0x0F0F,0xF0FF};
			_g->setColor(colors[_id % 4]);
			_g->fillRect(0,0,_w,_h);
			_g->update();
		}

		void Window::update(tCoord x,tCoord y,tSize width,tSize height) {
			_g->update(x,y,width,height);
		}
	}
}
