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
#include <gui/panel.h>
#include <gui/window.h>
#include <ostream>

using namespace std;

namespace gui {
	void Panel::onMousePressed(const MouseEvent &e) {
		passToCtrl(e,true);
	}
	void Panel::onMouseWheel(const MouseEvent &e) {
		passToCtrl(e,false);
	}

	void Panel::passToCtrl(const MouseEvent &e,bool focus) {
		Pos pos = e.getPos();
		for(auto it = _controls.begin(); it != _controls.end(); ++it) {
			shared_ptr<Control> c = *it;
			Pos cpos = c->getPos();
			if(pos.x >= cpos.x && pos.x < cpos.x + c->getSize().width &&
				pos.y >= cpos.y && pos.y < cpos.y + c->getSize().height) {
				MouseEvent ce(e.getType(),e.getXMovement(),e.getYMovement(),e.getWheelMovement(),
					  pos - cpos,e.getButtonMask());
				if(focus)
					_focus = c.get();
				if(e.getType() == MouseEvent::MOUSE_PRESSED)
					c->onMousePressed(ce);
				else
					c->onMouseWheel(ce);
				return;
			}
		}

		// if we're here the user has not clicked on a control, so set the focus to "nothing"
		if(focus)
			_focus = nullptr;
	}

	void Panel::layout() {
		_doingLayout = true;
		if(_layout)
			_layout->rearrange();
		for(auto it = _controls.begin(); it != _controls.end(); ++it)
			(*it)->layout();
		_doingLayout = false;
		getParent()->layoutChanged();
	}

	void Panel::resizeTo(const Size &size) {
		gpos_t diffw = getSize().width - size.width;
		gpos_t diffh = getSize().height - size.height;

		Control::resizeTo(size);
		if(diffw || diffh) {
			if(_layout) {
				_doingLayout = true;
				_layout->rearrange();
				_doingLayout = false;
				getParent()->layoutChanged();
			}
		}
	}

	void Panel::moveTo(const Pos &pos) {
		Pos cur = getPos();
		Control::moveTo(pos);

		// don't move the controls, their position is relative to us. just refresh the paint-region
		if(cur != pos) {
			for(auto it = _controls.begin(); it != _controls.end(); ++it)
				(*it)->setRegion();
		}
	}

	void Panel::setRegion() {
		Control::setRegion();
		for(auto it = _controls.begin(); it != _controls.end(); ++it)
			(*it)->setRegion();
	}

	void Panel::paint(Graphics &g) {
		// fill bg
		g.setColor(getTheme().getColor(Theme::CTRL_BACKGROUND));
		g.fillRect(Pos(0,0),getSize());

		// now paint controls
		for(auto it = _controls.begin(); it != _controls.end(); ++it) {
			shared_ptr<Control> c = *it;
			if(_updateRect.width) {
				sRectangle ctrlRect,inter;
				ctrlRect.x = c->getPos().x;
				ctrlRect.y = c->getPos().y;
				ctrlRect.width = c->getSize().width;
				ctrlRect.height = c->getSize().height;
				if(rectIntersect(&_updateRect,&ctrlRect,&inter)) {
					c->paintRect(*c->getGraphics(),
							Pos(inter.x - c->getPos().x,inter.y - c->getPos().y),
							Size(inter.width,inter.height));
				}
			}
			else
				c->repaint(false);
		}
	}

	void Panel::paintRect(Graphics &g,const Pos &pos,const Size &size) {
		_updateRect.x = pos.x;
		_updateRect.y = pos.y;
		_updateRect.width = size.width;
		_updateRect.height = size.height;
		UIElement::paintRect(g,pos,size);
		_updateRect.width = 0;
	}

	void Panel::add(shared_ptr<Control> c,Layout::pos_type pos) {
		_controls.push_back(c);
		c->setParent(this);
		if(_layout)
			_layout->add(this,c,pos);
	}

	void Panel::remove(shared_ptr<Control> c,Layout::pos_type pos) {
		_controls.erase_first(c);
		if(c.get() == _focus)
			setFocus(nullptr);
		if(_layout)
			_layout->remove(this,c,pos);
	}

	void Panel::removeAll() {
		_controls.clear();
		if(_focus)
			setFocus(nullptr);
		_layout->removeAll();
	}

	ostream &operator<<(ostream &s,const Panel &p) {
		s << "Panel[@" << p.getPos() << " size=" << p.getSize() << "]";
		return s;
	}
}
