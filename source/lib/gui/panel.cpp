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
	const Color Panel::DEF_BGCOLOR = Color(0x88,0x88,0x88);

	void Panel::onMousePressed(const MouseEvent &e) {
		gpos_t x = e.getX();
		gpos_t y = e.getY();
		for(vector<Control*>::iterator it = _controls.begin(); it != _controls.end(); ++it) {
			Control *c = *it;
			if(x >= c->getX() && x < c->getX() + c->getWidth() &&
				y >= c->getY() && y < c->getY() + c->getHeight()) {
				MouseEvent ce(e.getType(),e.getXMovement(),e.getYMovement(),
						x - c->getX(),y - c->getY(),e.getButtonMask());
				_focus = c;
				c->onMousePressed(ce);
				return;
			}
		}

		// if we're here the user has not clicked on a control, so set the focus to "nothing"
		_focus = NULL;
	}

	void Panel::layout() {
		if(_layout)
			_layout->rearrange();
		for(vector<Control*>::iterator it = _controls.begin(); it != _controls.end(); ++it)
			(*it)->layout();
	}

	void Panel::resizeTo(gsize_t width,gsize_t height) {
		gpos_t diffw = getWidth() - width;
		gpos_t diffh = getHeight() - height;

		Control::resizeTo(width,height);
		if(diffw || diffh) {
			if(_layout)
				_layout->rearrange();
		}
	}

	void Panel::moveTo(gpos_t x,gpos_t y) {
		gpos_t diffx = getX() - x;
		gpos_t diffy = getY() - y;
		Control::moveTo(x,y);

		// don't move the controls, their position is relative to us. just refresh the paint-region
		if(diffx || diffy) {
			for(vector<Control*>::iterator it = _controls.begin(); it != _controls.end(); ++it) {
				Control *c = *it;
				c->setRegion();
			}
		}
	}

	void Panel::setRegion() {
		Control::setRegion();
		for(vector<Control*>::iterator it = _controls.begin(); it != _controls.end(); ++it) {
			Control *c = *it;
			c->setRegion();
		}
	}

	void Panel::paint(Graphics &g) {
		// fill bg
		g.setColor(_bgColor);
		g.fillRect(0,0,getWidth(),getHeight());

		// now paint controls
		for(vector<Control*>::iterator it = _controls.begin(); it != _controls.end(); ++it) {
			Control *c = *it;
			if(_updateRect.width) {
				sRectangle ctrlRect,inter;
				ctrlRect.x = c->getX();
				ctrlRect.y = c->getY();
				ctrlRect.width = c->getWidth();
				ctrlRect.height = c->getHeight();
				if(rectIntersect(&_updateRect,&ctrlRect,&inter)) {
					c->paintRect(*c->getGraphics(),
							inter.x - c->getX(),inter.y - c->getY(),inter.width,inter.height);
				}
			}
			else
				c->paint(*c->getGraphics());
		}
	}

	void Panel::paintRect(Graphics &g,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		_updateRect.x = x;
		_updateRect.y = y;
		_updateRect.width = width;
		_updateRect.height = height;
		UIElement::paintRect(g,x,y,width,height);
		_updateRect.width = 0;
	}

	void Panel::add(Control &c,Layout::pos_type pos) {
		_controls.push_back(&c);
		c.setParent(this);
		if(_layout)
			_layout->add(this,&c,pos);
	}

	ostream &operator<<(ostream &s,const Panel &p) {
		s << "Panel[@" << p.getX() << "," << p.getY();
		s << " size=" << p.getWidth() << "," << p.getHeight() << "]";
		return s;
	}
}
