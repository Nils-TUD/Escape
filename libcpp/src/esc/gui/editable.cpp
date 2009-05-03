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
		void Editable::paint() {
			u32 cwidth = _g->getFont().getWidth();
			u32 cheight = _g->getFont().getHeight();
			_g->setColor(Color(255,255,255));
			_g->fillRect(1,1,getWidth() - 3,getHeight() - 3);
			_g->setColor(Color(0,0,0));
			_g->drawRect(0,0,getWidth() - 1,getHeight() - 1);

			tCoord ystart = (getHeight() - cheight) / 2;
			_g->drawString(PADDING,ystart,_str);
			if(_focused) {
				_g->fillRect(PADDING + cwidth * _cursor,ystart - CURSOR_OVERLAP,CURSOR_WIDTH,
						cheight + CURSOR_OVERLAP * 2);
			}

			_g->update();
		}

		void Editable::onFocusGained() {
			_focused = true;
			paint();
		}
		void Editable::onFocusLost() {
			_focused = false;
			paint();
		}

		void Editable::onKeyPressed(const KeyEvent &e) {
			if(e.isPrintable()) {
				_str.insert(_cursor,e.getCharacter());
				_cursor++;
				paint();
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
					paint();
			}
		}

		void Editable::onKeyReleased(const KeyEvent &e) {

		}
	}
}
