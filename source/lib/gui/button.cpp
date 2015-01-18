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

#include <gui/button.h>
#include <gui/control.h>
#include <sys/common.h>

namespace gui {
	Size Button::getPrefSize() const {
		gsize_t pad = getTheme().getTextPadding();
		return Size(getGraphics()->getFont().getStringWidth(_text) + pad * 2,
					getGraphics()->getFont().getSize().height + pad * 2);
	}

	void Button::onFocusGained() {
		Control::onFocusGained();
		setFocused(true);
	}
	void Button::onFocusLost() {
		Control::onFocusLost();
		setFocused(false);
	}

	void Button::onKeyPressed(const KeyEvent &e) {
		uchar keycode = e.getKeyCode();
		UIElement::onKeyPressed(e);
		if(keycode == VK_ENTER || keycode == VK_SPACE) {
			_clicked.send(*this);
			setPressed(true);
		}
	}
	void Button::onKeyReleased(const KeyEvent &e) {
		uchar keycode = e.getKeyCode();
		UIElement::onKeyReleased(e);
		if(keycode == VK_ENTER || keycode == VK_SPACE)
			setPressed(false);
	}

	void Button::onMousePressed(const MouseEvent &e) {
		UIElement::onMousePressed(e);
		if(!_pressed) {
			_clicked.send(*this);
			setPressed(true);
		}
	}
	void Button::onMouseReleased(const MouseEvent &e) {
		UIElement::onMouseReleased(e);
		if(_pressed)
			setPressed(false);
	}

	void Button::paintBackground(Graphics &g) {
		Size size = getSize();
		const Color &col = getTheme().getColor(Theme::BTN_BACKGROUND);
		g.colorFadeRect(VERTICAL,col,col + 20,1,1,size.width - 2,size.height - 2);
	}

	void Button::paintBorder(Graphics &g) {
		Size size = getSize();
		g.setColor(getTheme().getColor(Theme::CTRL_LIGHTBORDER));
		g.drawLine(0,0,size.width - 1,0);
		if(_focused)
			g.drawLine(0,1,size.width - 1,1);
		g.drawLine(0,0,0,size.height - 1);
		if(_focused)
			g.drawLine(1,0,1,size.height - 1);

		g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
		g.drawLine(size.width - 1,0,size.width - 1,size.height - 1);
		if(_focused)
			g.drawLine(size.width - 2,0,size.width - 2,size.height - 1);
		g.drawLine(0,size.height - 1,size.width - 1,size.height - 1);
		if(_focused)
			g.drawLine(0,size.height - 2,size.width - 1,size.height - 2);
	}

	void Button::paint(Graphics &g) {
		paintBackground(g);
		paintBorder(g);

		if(!_text.empty()) {
			Size size = getSize();
			g.setColor(getTheme().getColor(Theme::BTN_FOREGROUND));
			gsize_t count = g.getFont().limitStringTo(_text,size.width - 2);
			if(_pressed) {
				g.drawString((size.width - g.getFont().getStringWidth(_text.substr(0,count))) / 2 + 1,
						(size.height - g.getFont().getSize().height) / 2 + 1,_text,0,count);
			}
			else {
				g.drawString((size.width - g.getFont().getStringWidth(_text.substr(0,count))) / 2,
						(size.height - g.getFont().getSize().height) / 2,_text,0,count);
			}
		}
	}
}
