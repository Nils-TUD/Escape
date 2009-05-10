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
#include <esc/gui/combobox.h>
#include <esc/gui/control.h>
#include <esc/gui/color.h>

namespace esc {
	namespace gui {
		const Color ComboBox::BORDERCOLOR = Color(0,0,0);
		const Color ComboBox::ITEM_BGCOLOR = Color(0xFF,0xFF,0xFF);
		const Color ComboBox::ITEM_FGCOLOR = Color(0,0,0);
		const Color ComboBox::BTN_BGCOLOR = Color(0x80,0x80,0x80);
		const Color ComboBox::BTN_ARROWCOLOR = Color(0xFF,0xFF,0xFF);

		const Color ComboBox::ItemWindow::SEL_BGCOLOR = Color(0,0,0xFF);
		const Color ComboBox::ItemWindow::SEL_FGCOLOR = Color(0xFF,0xFF,0xFF);

		void ComboBox::ItemWindow::close(tCoord x,tCoord y) {
			// just do this here if we will not receive a mouse-click for the combobox-button
			// anyway
			tCoord cbx = _cb->getX(), wx = _cb->getWindow().getX();
			tCoord cby = _cb->getY(), wy = _cb->getWindow().getY();
			tSize cbw = _cb->getWidth();
			tSize cbh = _cb->getHeight();
			tSize tbh = _cb->getWindow().getTitleBarHeight();
			if(!(x >= wx + cbx + cbw - cbh && x < wx + cbx + cbw &&
				y >= wy + cby + tbh && y < wy + cby + tbh + cbh)) {
				closeImpl();
			}
		}

		void ComboBox::ItemWindow::paint(Graphics &g) {
			Window::paint(g);
			g.setColor(ComboBox::ITEM_BGCOLOR);
			g.fillRect(0,0,getWidth(),getHeight());

			g.setColor(ComboBox::BORDERCOLOR);
			g.drawRect(0,0,getWidth(),getHeight());

			g.setColor(ITEM_FGCOLOR);
			tSize itemHeight = g.getFont().getHeight();
			tCoord y = 0;
			for(u32 i = 0; i < _cb->_items.size(); i++) {
				if(_highlighted == (s32)i) {
					g.setColor(SEL_BGCOLOR);
					g.fillRect(1,y + 1,getWidth() - 2,itemHeight + PADDING * 2 - 2);
					g.setColor(SEL_FGCOLOR);
				}
				g.drawString(PADDING,y + PADDING,_cb->_items[i]);
				g.setColor(ITEM_FGCOLOR);
				y += itemHeight + PADDING * 2;
			}
		}

		void ComboBox::ItemWindow::onMouseMoved(const MouseEvent &e) {
			s32 item = getItemAt(e.getX(),e.getY());
			if(item != _highlighted) {
				_highlighted = item;
				repaint();
			}
		}

		void ComboBox::ItemWindow::onMouseReleased(const MouseEvent &e) {
			_cb->_selected = _highlighted;
			closeImpl();
		}

		s32 ComboBox::ItemWindow::getItemAt(tCoord x,tCoord y) {
			return y / (getGraphics()->getFont().getHeight() + PADDING * 2);
		}

		void ComboBox::ItemWindow::closeImpl() {
			delete this;
			// notify combo
			_cb->_pressed = false;
			_cb->_win = NULL;
			_cb->repaint();
		}

		void ComboBox::paint(Graphics &g) {
			u32 btnWidth = getHeight();
			// paint item
			g.setColor(ITEM_BGCOLOR);
			g.fillRect(1,1,getWidth() - btnWidth - 2,getHeight() - 2);
			g.setColor(ITEM_FGCOLOR);
			g.drawRect(0,0,getWidth() - btnWidth,getHeight());
			if(_selected >= 0) {
				tCoord ystart = (getHeight() - g.getFont().getHeight()) / 2;
				g.drawString(2,ystart,_items[_selected]);
			}

			// paint button border and bg
			g.setColor(BTN_BGCOLOR);
			g.fillRect(getWidth() - btnWidth + 2,1,btnWidth - 3,getHeight() - 2);
			g.setColor(BORDERCOLOR);
			g.drawRect(getWidth() - btnWidth + 1,0,btnWidth - 1,getHeight());

			// paint cross
			u32 arrowPad = 4;
			u32 pressedPad = _pressed ? 1 : 0;
			g.setColor(BTN_ARROWCOLOR);
			g.drawLine(getWidth() - btnWidth + arrowPad,arrowPad + pressedPad,
					getWidth() - arrowPad,arrowPad + pressedPad);
			g.drawLine(getWidth() - btnWidth + arrowPad,arrowPad + pressedPad,
					getWidth() - btnWidth / 2,getHeight() - arrowPad + pressedPad);
			g.drawLine(getWidth() - arrowPad,arrowPad + pressedPad,
					getWidth() - btnWidth / 2,getHeight() - arrowPad + pressedPad);
		}

		void ComboBox::onMousePressed(const MouseEvent &e) {
			if(e.getX() >= getX() + getWidth() - getHeight() && !_pressed) {
				_pressed = true;
				repaint();
				if(_win) {
					delete _win;
					_win = NULL;
				}
				else {
					Window &w = getWindow();
					_win = new ItemWindow(this,w.getX() + getX(),
							w.getY() + w.getTitleBarHeight() + getY() + getHeight(),getWidth(),
							_items.size() * (getGraphics()->getFont().getHeight() + ItemWindow::PADDING * 2));
				}
			}
		}
		void ComboBox::onMouseReleased(const MouseEvent &e) {
			if(e.getX() >= getX() + getWidth() - getHeight() + 2 && _pressed) {
				_pressed = false;
				repaint();
			}
		}
	}
}
