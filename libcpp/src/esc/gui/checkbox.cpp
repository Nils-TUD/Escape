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
#include <esc/gui/checkbox.h>
#include <esc/gui/control.h>

namespace esc {
	namespace gui {
		Color Checkbox::FGCOLOR = Color(0xFF,0xFF,0xFF);
		Color Checkbox::BGCOLOR = Color(0x88,0x88,0x88);
		Color Checkbox::BOX_COLOR = Color(0x20,0x20,0x20);

		Checkbox &Checkbox::operator=(const Checkbox &b) {
			// ignore self-assignments
			if(this == &b)
				return *this;
			Control::operator=(b);
			_focused = false;
			_checked = b._checked;
			_text = b._text;
			return *this;
		}

		void Checkbox::onFocusGained() {
			_focused = true;
			repaint();
		}
		void Checkbox::onFocusLost() {
			_focused = false;
			repaint();
		}

		void Checkbox::onKeyReleased(const KeyEvent &e) {
			u8 keycode = e.getKeyCode();
			UIElement::onKeyReleased(e);
			if(keycode == VK_ENTER || keycode == VK_SPACE)
				setChecked(!_checked);
		}

		void Checkbox::onMouseReleased(const MouseEvent &e) {
			UIElement::onMouseReleased(e);
			setChecked(!_checked);
		}

		void Checkbox::setChecked(bool checked) {
			_checked = checked;
			repaint();
		}

		void Checkbox::paint(Graphics &g) {
			u32 cheight = g.getFont().getHeight();
			u32 boxSize = getHeight() - 2;

			g.setColor(BGCOLOR);
			g.fillRect(0,0,getWidth(),getHeight());

			g.setColor(BOX_COLOR);
			g.drawRect(1,1,boxSize - 2,boxSize - 2);

			if(_checked) {
				g.drawLine(2,2,boxSize - 2,boxSize - 2);
				g.drawLine(boxSize - 2,2,2,boxSize - 2);
			}

			g.setColor(FGCOLOR);
			g.drawString(boxSize + 2,(getHeight() - cheight) / 2 + 1,_text);
		}
	}
}
