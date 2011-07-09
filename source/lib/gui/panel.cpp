/**
 * $Id: window.cpp 968 2011-07-07 18:20:21Z nasmussen $
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
#include <gui/panel.h>
#include <gui/window.h>
#include <ostream>

namespace gui {
	void Panel::onMousePressed(const MouseEvent &e) {
		// MouseEvent::MOUSE_PRESSED
		gpos_t x = e.getX() - getX();
		gpos_t y = e.getY() - getY();
		for(vector<Control*>::iterator it = _controls.begin(); it != _controls.end(); ++it) {
			Control *c = *it;
			if(x >= c->getX() && x < c->getX() + c->getWidth() &&
				y >= c->getY() && y < c->getY() + c->getHeight()) {
				_focus = c;
				c->onMousePressed(e);
				return;
			}
		}

		// if we're here the user has not clicked on a control, so set the focus to "nothing"
		_focus = NULL;
	}

	void Panel::moveTo(gpos_t x,gpos_t y) {
		Control::moveTo(x,y);

		x -= getX();
		y -= getY();
		for(vector<Control*>::iterator it = _controls.begin(); it != _controls.end(); ++it) {
			Control *c = *it;
			c->moveTo(x,y);
		}
	}

	void Panel::paint(Graphics &g) {
		// fill bg
		g.setColor(_bgColor);
		g.fillRect(0,0,getWidth(),getHeight());

		// now paint controls
		for(vector<Control*>::iterator it = _controls.begin(); it != _controls.end(); ++it) {
			Control *c = *it;
			c->paint(*c->getGraphics());
		}
	}

	void Panel::add(Control &c) {
		_controls.push_back(&c);
		c.setParent(this);
	}

	ostream &operator<<(ostream &s,const Panel &p) {
		s << "Panel[@" << p.getX() << "," << p.getY();
		s << " size=" << p.getWidth() << "," << p.getHeight() << "]";
		return s;
	}
}
