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
#include <gui/button.h>
#include <gui/control.h>

namespace gui {
	gsize_t Button::getMinWidth() const {
		return getGraphics()->getFont().getStringWidth(_text) + getTheme().getTextPadding() * 2;
	}
	gsize_t Button::getMinHeight() const {
		return getGraphics()->getFont().getHeight() + getTheme().getTextPadding() * 2;
	}

	void Button::onFocusGained() {
		Control::onFocusGained();
		_focused = true;
		repaint();
	}
	void Button::onFocusLost() {
		Control::onFocusLost();
		_focused = false;
		repaint();
	}

	void Button::onKeyPressed(const KeyEvent &e) {
		uchar keycode = e.getKeyCode();
		UIElement::onKeyPressed(e);
		if(keycode == VK_ENTER || keycode == VK_SPACE) {
			notifyListener();
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
			notifyListener();
			setPressed(true);
		}
	}
	void Button::onMouseReleased(const MouseEvent &e) {
		UIElement::onMouseReleased(e);
		if(_pressed)
			setPressed(false);
	}

	void Button::setPressed(bool pressed) {
		_pressed = pressed;
		repaint();
	}

	void Button::notifyListener() {
		for(vector<ActionListener*>::iterator it = _listener.begin(); it != _listener.end(); ++it)
			(*it)->actionPerformed(*this);
	}

	void Button::paintBackground(Graphics &g) {
		g.setColor(getTheme().getColor(Theme::BTN_BACKGROUND));
		g.fillRect(1,1,getWidth() - 2,getHeight() - 2);
	}

	void Button::paintBorder(Graphics &g) {
		g.setColor(getTheme().getColor(Theme::CTRL_LIGHTBORDER));
		g.drawLine(0,0,getWidth() - 1,0);
		if(_focused)
			g.drawLine(0,1,getWidth() - 1,1);
		g.drawLine(0,0,0,getHeight() - 1);
		if(_focused)
			g.drawLine(1,0,1,getHeight() - 1);

		g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
		g.drawLine(getWidth() - 1,0,getWidth() - 1,getHeight() - 1);
		if(_focused)
			g.drawLine(getWidth() - 2,0,getWidth() - 2,getHeight() - 1);
		g.drawLine(0,getHeight() - 1,getWidth() - 1,getHeight() - 1);
		if(_focused)
			g.drawLine(0,getHeight() - 2,getWidth() - 1,getHeight() - 2);
	}

	void Button::paint(Graphics &g) {
		paintBackground(g);
		paintBorder(g);

		g.setColor(getTheme().getColor(Theme::BTN_FOREGROUND));
		gsize_t count = g.getFont().limitStringTo(_text,getWidth() - 2);
		if(_pressed) {
			g.drawString((getWidth() - g.getFont().getStringWidth(_text.substr(0,count))) / 2 + 1,
					(getHeight() - g.getFont().getHeight()) / 2 + 1,_text,0,count);
		}
		else {
			g.drawString((getWidth() - g.getFont().getStringWidth(_text.substr(0,count))) / 2,
					(getHeight() - g.getFont().getHeight()) / 2,_text,0,count);
		}
	}
}
