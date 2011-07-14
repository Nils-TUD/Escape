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
#include <gui/combobox.h>
#include <gui/control.h>
#include <gui/color.h>
#include <iterator>

namespace gui {
	void ComboBox::ItemWindow::close(gpos_t x,gpos_t y) {
		// just do this here if we will not receive a mouse-click for the combobox-button
		// anyway
		gpos_t cbx = _cb->getX(), wx = _cb->getWindow()->getX();
		gpos_t cby = _cb->getY(), wy = _cb->getWindow()->getY();
		gsize_t cbw = _cb->getWidth();
		gsize_t cbh = _cb->getHeight();
		gsize_t tbh = _cb->getWindow()->getTitleBarHeight();
		if(!(x >= wx + cbx + cbw - cbh && x < wx + cbx + cbw &&
			y >= wy + cby + tbh && y < wy + cby + tbh + cbh)) {
			closeImpl();
		}
	}

	void ComboBox::ItemWindow::paint(Graphics &g) {
		Window::paint(g);
		g.setColor(getTheme().getColor(Theme::TEXT_BACKGROUND));
		g.fillRect(0,0,getWidth(),getHeight());

		g.setColor(getTheme().getColor(Theme::CTRL_BORDER));
		g.drawRect(0,0,getWidth(),getHeight());

		const Color &tf = getTheme().getColor(Theme::TEXT_FOREGROUND);
		const Color &sb = getTheme().getColor(Theme::SEL_BACKGROUND);
		const Color &sf = getTheme().getColor(Theme::SEL_FOREGROUND);

		gpos_t y = 0;
		gsize_t itemHeight = g.getFont().getHeight();
		g.setColor(tf);
		for(vector<string>::iterator it = _cb->_items.begin(); it != _cb->_items.end(); ++it) {
			if(_highlighted == (int)std::distance(_cb->_items.begin(),it)) {
				g.setColor(sb);
				g.fillRect(SELPAD,y + SELPAD,
						getWidth() - SELPAD * 2,
						itemHeight + getTheme().getTextPadding() * 2 - SELPAD * 2);
				g.setColor(sf);
			}
			g.drawString(getTheme().getTextPadding(),y + getTheme().getTextPadding(),*it);
			g.setColor(tf);
			y += itemHeight + getTheme().getTextPadding() * 2;
		}
	}

	void ComboBox::ItemWindow::onMouseMoved(const MouseEvent &e) {
		int item = getItemAt(e.getX(),e.getY());
		if(item < (int)_cb->_items.size() && item != _highlighted) {
			_highlighted = item;
			repaint();
		}
	}

	void ComboBox::ItemWindow::onMouseReleased(const MouseEvent &e) {
		UNUSED(e);
		_cb->_selected = _highlighted;
		closeImpl();
	}

	int ComboBox::ItemWindow::getItemAt(gpos_t x,gpos_t y) {
		UNUSED(x);
		return y / (getGraphics()->getFont().getHeight() + getTheme().getTextPadding() * 2);
	}

	void ComboBox::ItemWindow::closeImpl() {
		delete this;
		// notify combo
		_cb->_pressed = false;
		_cb->_win = NULL;
		_cb->repaint();
	}

	gsize_t ComboBox::getMinWidth() const {
		gsize_t max = 0;
		const Font& f = getGraphics()->getFont();
		for(vector<string>::const_iterator it = _items.begin(); it != _items.end(); ++it) {
			gsize_t width = f.getStringWidth(*it);
			if(width > max)
				max = width;
			if(max > MAX_WIDTH) {
				max = MAX_WIDTH;
				break;
			}
		}
		return max + getTheme().getTextPadding() * 2 + getGraphics()->getFont().getHeight() + ARROW_PAD;
	}
	gsize_t ComboBox::getMinHeight() const {
		return getGraphics()->getFont().getHeight() + getTheme().getTextPadding() * 2;
	}

	void ComboBox::paint(Graphics &g) {
		gsize_t btnWidth = getHeight();
		// paint item
		g.setColor(getTheme().getColor(Theme::TEXT_BACKGROUND));
		g.fillRect(1,1,getWidth() - btnWidth - 2,getHeight() - 2);
		g.setColor(getTheme().getColor(Theme::TEXT_FOREGROUND));
		g.drawRect(0,0,getWidth() - btnWidth,getHeight());
		if(_selected >= 0) {
			gpos_t ystart = (getHeight() - g.getFont().getHeight()) / 2;
			g.drawString(getTheme().getTextPadding(),ystart,_items[_selected]);
		}

		// paint button border and bg
		g.setColor(getTheme().getColor(Theme::BTN_BACKGROUND));
		g.fillRect(getWidth() - btnWidth + 2,1,btnWidth - 3,getHeight() - 2);
		g.setColor(getTheme().getColor(Theme::CTRL_BORDER));
		g.drawRect(getWidth() - btnWidth + 1,0,btnWidth - 1,getHeight());

		// paint triangle
		size_t pressedPad = _pressed ? 1 : 0;
		g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
		g.drawLine(getWidth() - btnWidth + ARROW_PAD,ARROW_PAD + pressedPad,
				getWidth() - ARROW_PAD,ARROW_PAD + pressedPad);
		g.drawLine(getWidth() - btnWidth + ARROW_PAD,ARROW_PAD + pressedPad,
				getWidth() - btnWidth / 2,getHeight() - ARROW_PAD + pressedPad);
		g.drawLine(getWidth() - ARROW_PAD,ARROW_PAD + pressedPad,
				getWidth() - btnWidth / 2,getHeight() - ARROW_PAD + pressedPad);
	}

	void ComboBox::onMousePressed(const MouseEvent &e) {
		UIElement::onMousePressed(e);
		if(!_pressed) {
			_pressed = true;
			repaint();
			if(_win) {
				delete _win;
				_win = NULL;
			}
			else {
				gsize_t pad = Application::getInstance()->getDefaultTheme()->getTextPadding();
				gsize_t height = _items.size() * (getGraphics()->getFont().getHeight() + pad * 2);
				const Window *w = getWindow();
				_win = new ItemWindow(this,w->getX() + getWindowX(),
						w->getY() + getWindowY() + getHeight(),getWidth(),height);
			}
		}
	}
	void ComboBox::onMouseReleased(const MouseEvent &e) {
		UIElement::onMouseReleased(e);
		if(e.getX() >= getX() + getWidth() - getHeight() + 2 && _pressed) {
			_pressed = false;
			repaint();
		}
	}
}
