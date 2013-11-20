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
#include <gui/graphics/color.h>
#include <gui/combobox.h>
#include <gui/control.h>
#include <iterator>

using namespace std;

namespace gui {
	void ComboBox::ItemWindow::close(const Pos &pos) {
		// just do this here if we will not receive a mouse-click for the combobox-button
		// anyway
		Pos cb = _cb->getPos();
		Pos w = _cb->getWindow()->getPos();
		Size cbs = _cb->getSize();
		gsize_t tbh = _cb->getWindow()->getTitleBarHeight();
		if(!(pos.x >= (gpos_t)(w.x + cb.x + cbs.width - cbs.height) &&
				pos.x < (gpos_t)(w.x + cb.x + cbs.width) &&
				pos.y >= (gpos_t)(w.y + cb.y + tbh) &&
				pos.y < (gpos_t)(w.y + cb.y + tbh + cbs.height))) {
			closeImpl();
		}
	}

	void ComboBox::ItemWindow::paint(Graphics &g) {
		g.setColor(getTheme().getColor(Theme::TEXT_BACKGROUND));
		g.fillRect(Pos(0,0),getSize());

		g.setColor(getTheme().getColor(Theme::CTRL_BORDER));
		g.drawRect(Pos(0,0),getSize());

		const Color &tf = getTheme().getColor(Theme::TEXT_FOREGROUND);
		const Color &sb = getTheme().getColor(Theme::SEL_BACKGROUND);
		const Color &sf = getTheme().getColor(Theme::SEL_FOREGROUND);

		gpos_t y = 0;
		gsize_t itemHeight = g.getFont().getSize().height;
		g.setColor(tf);
		for(auto it = _cb->_items.begin(); it != _cb->_items.end(); ++it) {
			if(_highlighted == (int)distance(_cb->_items.begin(),it)) {
				g.colorFadeRect(VERTICAL,sb,sb + 20,SELPAD,y + SELPAD,
						getSize().width - SELPAD * 2 - 1,
						itemHeight + getTheme().getTextPadding() * 2 - SELPAD * 2);
				g.setColor(sf);
			}
			g.drawString(getTheme().getTextPadding(),y + getTheme().getTextPadding(),*it);
			g.setColor(tf);
			y += itemHeight + getTheme().getTextPadding() * 2;
		}
	}

	void ComboBox::ItemWindow::onMouseMoved(const MouseEvent &e) {
		int item = getItemAt(e.getPos());
		if(item < (int)_cb->_items.size() && item != _highlighted) {
			_highlighted = item;
			makeDirty(true);
			repaint();
		}
	}

	void ComboBox::ItemWindow::onMouseReleased(A_UNUSED const MouseEvent &e) {
		_cb->setSelectedIndex(_highlighted);
		closeImpl();
	}

	int ComboBox::ItemWindow::getItemAt(const Pos &pos) {
		return pos.y / (getGraphics()->getFont().getSize().height + getTheme().getTextPadding() * 2);
	}

	void ComboBox::ItemWindow::closeImpl() {
		_cb->removeWindow();
		_cb->setPressed(false);
	}

	Size ComboBox::getPrefSize() const {
		gsize_t max = 0;
		gsize_t pad = getTheme().getTextPadding();
		const Font& f = getGraphics()->getFont();
		for(auto it = _items.begin(); it != _items.end(); ++it) {
			gsize_t width = f.getStringWidth(*it);
			if(width > max)
				max = width;
			if(max > MAX_WIDTH) {
				max = MAX_WIDTH;
				break;
			}
		}
		return Size(max + pad * 2 + f.getSize().height + ARROW_PAD,f.getSize().height + pad * 2);
	}

	void ComboBox::paint(Graphics &g) {
		Size size = getSize();
		gsize_t btnWidth = size.height;
		gsize_t textWidth = size.width - btnWidth;
		// paint item
		g.setColor(getTheme().getColor(Theme::TEXT_BACKGROUND));
		g.fillRect(1,1,textWidth - 2,size.height - 2);
		g.setColor(getTheme().getColor(Theme::TEXT_FOREGROUND));
		g.drawRect(0,0,textWidth,size.height);
		if(_selected >= 0) {
			gsize_t textlimit = textWidth - getTheme().getTextPadding() * 2;
			gpos_t ystart = (size.height - g.getFont().getSize().height) / 2;
			gsize_t count = getGraphics()->getFont().limitStringTo(_items[_selected],textlimit);
			g.drawString(getTheme().getTextPadding(),ystart,_items[_selected],0,count);
		}

		// paint button border and bg
		const Color &bg = getTheme().getColor(Theme::BTN_BACKGROUND);
		g.colorFadeRect(VERTICAL,bg,bg + 20,textWidth + 2,1,btnWidth - 3,size.height - 2);
		g.setColor(getTheme().getColor(Theme::CTRL_BORDER));
		g.drawRect(textWidth + 1,0,btnWidth - 1,size.height);

		// paint triangle
		size_t pressedPad = _pressed ? 1 : 0;
		Pos t1(size.width - btnWidth / 2,size.height - ARROW_PAD + pressedPad);
		Pos t2(size.width - ARROW_PAD,ARROW_PAD + pressedPad);
		Pos t3(textWidth + ARROW_PAD,ARROW_PAD + pressedPad);
		g.setColor(getTheme().getColor(Theme::CTRL_DARKBACK));
		g.fillTriangle(t1,t2,t3);
		g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
		g.drawTriangle(t1,t2,t3);
	}

	void ComboBox::onMousePressed(const MouseEvent &e) {
		UIElement::onMousePressed(e);
		if(!_pressed) {
			setPressed(true);
			if(_win)
				removeWindow();
			else {
				gsize_t pad = Application::getInstance()->getDefaultTheme()->getTextPadding();
				gsize_t height = _items.size() * (getGraphics()->getFont().getSize().height + pad * 2);
				const Window *w = getWindow();
				Pos pos(w->getPos().x + getWindowPos().x,
				        w->getPos().y + getWindowPos().y + getSize().height);
				_win = make_control<ItemWindow>(this,pos,Size(getSize().width,height));
				_win->show();
				Application::getInstance()->addWindow(_win);
			}
		}
	}
	void ComboBox::onMouseReleased(const MouseEvent &e) {
		UIElement::onMouseReleased(e);
		if(_pressed && e.getPos().x >= (gpos_t)(getPos().x + getSize().width - getSize().height + 2))
			setPressed(false);
	}
}
