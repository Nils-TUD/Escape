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
#include <esc/gui/common.h>
#include <esc/gui/editable.h>
#include <esc/keycodes.h>

namespace esc {
	namespace gui {
		const Color Editable::BGCOLOR = Color(0xFF,0xFF,0xFF);
		const Color Editable::FGCOLOR = Color(0,0,0);
		const Color Editable::BORDER_COLOR = Color(0,0,0);
		const Color Editable::CURSOR_COLOR = Color(0,0,0);

		Editable &Editable::operator=(const Editable &e) {
			// ignore self-assigments
			if(this == &e)
				return *this;
			Control::operator=(e);
			_cursor = e._cursor;
			_focused = false;
			_str = e._str;
			return *this;
		}

		void Editable::paint(Graphics &g) {
			u32 cwidth = g.getFont().getWidth();
			u32 cheight = g.getFont().getHeight();
			g.setColor(BGCOLOR);
			g.fillRect(1,1,getWidth() - 2,getHeight() - 2);
			g.setColor(BORDER_COLOR);
			g.drawRect(0,0,getWidth(),getHeight());

			tCoord ystart = (getHeight() - cheight) / 2;
			g.setColor(FGCOLOR);
			g.drawString(PADDING,ystart,_str);
			if(_focused) {
				g.setColor(CURSOR_COLOR);
				g.fillRect(PADDING + cwidth * _cursor,ystart - CURSOR_OVERLAP,CURSOR_WIDTH,
						cheight + CURSOR_OVERLAP * 2);
			}
		}

		void Editable::onFocusGained() {
			_focused = true;
			repaint();
		}
		void Editable::onFocusLost() {
			_focused = false;
			repaint();
		}

		void Editable::onKeyPressed(const KeyEvent &e) {
			UIElement::onKeyPressed(e);
			if(e.isPrintable()) {
				_str.insert(_cursor,e.getCharacter());
				_cursor++;
				repaint();
			}
			else {
				bool changed = false;
				switch(e.getKeyCode()) {
					case VK_DELETE:
						if(_cursor < _str.length()) {
							_str.erase(_cursor,1);
							changed = true;
						}
						break;
					case VK_LEFT:
						if(_cursor > 0) {
							_cursor--;
							changed = true;
						}
						break;
					case VK_RIGHT:
						if(_cursor < _str.length()) {
							_cursor++;
							changed = true;
						}
						break;
					case VK_HOME:
						if(_cursor > 0) {
							_cursor = 0;
							changed = true;
						}
						break;
					case VK_END:
						if(_cursor < _str.length()) {
							_cursor = _str.length();
							changed = true;
						}
						break;
					case VK_BACKSP:
						if(_cursor > 0) {
							_str.erase(_cursor - 1,1);
							_cursor--;
							changed = true;
						}
						break;
				}

				if(changed)
					repaint();
			}
		}
	}
}
