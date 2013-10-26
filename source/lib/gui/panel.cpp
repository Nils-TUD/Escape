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
#include <functor.h>
#include <typeinfo>
#include <ostream>
#include <iomanip>

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
			if(pos.x >= cpos.x && pos.x < (gpos_t)(cpos.x + c->getSize().width) &&
				pos.y >= cpos.y && pos.y < (gpos_t)(cpos.y + c->getSize().height)) {
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

	bool Panel::layout() {
		bool res = false;
		_doingLayout = true;
		if(_layout)
			res |= _layout->rearrange();
		for(auto it = _controls.begin(); it != _controls.end(); ++it)
			res |= (*it)->layout();
		_doingLayout = false;
		if(res)
			getParent()->layoutChanged();
		return res;
	}

	bool Panel::resizeTo(const Size &size) {
		gpos_t diffw = getSize().width - size.width;
		gpos_t diffh = getSize().height - size.height;

		Control::resizeTo(size);
		if(diffw || diffh) {
			if(_layout) {
				_doingLayout = true;
				_layout->rearrange();
				_doingLayout = false;
				getParent()->layoutChanged();
				return true;
			}
		}
		return false;
	}

	bool Panel::moveTo(const Pos &pos) {
		Pos cur = getPos();
		Control::moveTo(pos);

		// don't move the controls, their position is relative to us. just refresh the paint-region
		if(cur != pos) {
			for(auto it = _controls.begin(); it != _controls.end(); ++it)
				(*it)->setRegion();
			return true;
		}
		return false;
	}

	void Panel::setRegion() {
		Control::setRegion();
		for(auto it = _controls.begin(); it != _controls.end(); ++it)
			(*it)->setRegion();
	}

	void Panel::paint(Graphics &g) {
		bool dirty = UIElement::isDirty();
		Rectangle prect = g.getPaintRect();
		bool ispart = prect.getSize() != getSize();
		// fill bg
		g.setColor(getTheme().getColor(Theme::CTRL_BACKGROUND));
		g.fillRect(prect.getPos(),prect.getSize());

		// now paint controls
		for(auto it = _controls.begin(); it != _controls.end(); ++it) {
			shared_ptr<Control> c = *it;
			c->makeDirty(dirty);
			if(!c->isDirty() || ispart) {
				Rectangle ctrlRect(c->getPos(),c->getSize());
				Rectangle inter = intersection(ctrlRect,prect);
				if(!inter.empty()) {
					c->makeDirty(true);
					c->repaintRect(inter.getPos() - c->getPos(),inter.getSize(),false);
				}
			}
			else
				c->repaint(false);
		}
	}

	void Panel::add(shared_ptr<Control> c,Layout::pos_type pos) {
		_controls.push_back(c);
		c->setParent(this);
		if(_layout)
			_layout->add(this,c,pos);
		makeDirty(true);
	}

	void Panel::remove(shared_ptr<Control> c,Layout::pos_type pos) {
		_controls.erase_first(c);
		if(c.get() == _focus)
			setFocus(nullptr);
		if(_layout)
			_layout->remove(this,c,pos);
		makeDirty(true);
	}

	void Panel::removeAll() {
		_controls.clear();
		if(_focus)
			setFocus(nullptr);
		_layout->removeAll();
		makeDirty(true);
	}

	void Panel::layoutChanged() {
		if(_doingLayout)
			return;
		if(layout())
			repaint();
	}

	void Panel::print(std::ostream &os, bool rec, size_t indent) const {
		UIElement::print(os, rec, indent);
		os << " layout=" << (_layout ? typeid(*_layout).name() : "(null)");
		if(rec) {
			os << " {\n";
			for(auto it = _controls.begin(); it != _controls.end(); ++it) {
				(*it)->print(os,rec,indent + 2);
				os << '\n';
			}
			os << std::setw(indent) << "" << "}";
		}
	}
}
