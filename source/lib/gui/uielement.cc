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

#include <gui/graphics/graphics.h>
#include <gui/control.h>
#include <gui/uielement.h>
#include <gui/window.h>
#include <sys/common.h>
#include <iomanip>
#include <sstream>
#include <typeinfo>

using namespace std;

namespace gui {
	UIElement::id_type UIElement::_nextid = 1;

	Window *UIElement::getWindow() {
		UIElement *p = _parent;
		// if its the window itself
		if(!p)
			return reinterpret_cast<Window*>(this);
		// otherwise go up until we've found the window
		while(p->_parent)
			p = p->_parent;
		return reinterpret_cast<Window*>(p);
	}

	Pos UIElement::getWindowPos() const {
		if(_parent) {
			if(_parent->_parent)
				return _parent->getWindowPos() + _pos;
			return _pos;
		}
		return Pos();
	}
	Pos UIElement::getScreenPos() const {
		if(_parent)
			return _parent->getScreenPos() + _pos;
		return _pos;
	}

	void UIElement::onMouseMoved(const MouseEvent &e) {
		_mouseMoved.send(*this,e);
	}
	void UIElement::onMouseReleased(const MouseEvent &e) {
		_mouseReleased.send(*this,e);
	}
	void UIElement::onMousePressed(const MouseEvent &e) {
		_mousePressed.send(*this,e);
	}
	void UIElement::onMouseWheel(const MouseEvent &e) {
		_mouseWheel.send(*this,e);
	}
	void UIElement::onKeyPressed(const KeyEvent &e) {
		_keyPressed.send(*this,e);
	}
	void UIElement::onKeyReleased(const KeyEvent &e) {
		_keyReleased.send(*this,e);
	}

	void UIElement::setFocus(A_UNUSED Control *c) {
		// by default its ignored
	}

	void UIElement::debug() {
#if defined(DEBUG_GUI)
		static int pos = 0;
		ostringstream ostr;
		ostr << _id;
		_g->setColor(Color(0xFF,0,0));

		gpos_t x = 0, y = 0;
		switch(pos++ % 3) {
			case 1:
				x = getSize().width - _g->getFont().getStringWidth(ostr.str());
				y = getSize().height - _g->getFont().getSize().height;
				break;
			case 2:
				x = getSize().width / 2 - _g->getFont().getStringWidth(ostr.str()) / 2;
				y = getSize().height / 2 - _g->getFont().getSize().height / 2;
				break;
		}
		_g->drawString(x,y,ostr.str());

		_g->drawRect(Pos(0,0),getSize());

		Window *win = _g->getBuffer()->getWindow();
		if(win->hasTitleBar())
			cout << "[" << win->getTitle();
		else
			cout << "[#" << win->getId();
		cout << ":" << getId() << ":" << typeid(*this).name() << "]";
		cout << " @" << getPos() << " size:" << getSize() << " pref:" << getPreferredSize() << endl;
#endif
	}

	void UIElement::requestUpdate() {
		if(_g)
			_g->requestUpdate();
	}

	void UIElement::repaint(bool update) {
		if(_g && isDirty()) {
			Window *win;
			if(_parent && (win = getWindow()) && !win->_repainting)
				win->repaintRect(getWindowPos(),getSize(),update);
			else {
				paint(*_g);
				debug();
				if(update)
					_g->requestUpdate();
			}
			makeClean();
		}
	}

	void UIElement::paintRect(Graphics &g,const Pos &pos,const Size &size) {
		// save old rectangle
		Pos goff = g.getMinOff();
		Size gs = g.getSize();

		// change g to cover only the rectangle to repaint
		Rectangle rect(g.getPos() + pos,size);
		if(_parent)
			rect = _parent->getVisibleRect(rect);
		g.setMinOff(rect.getPos());
		g.setSize(rect.getSize());
		// now let the ui-element paint it; this does not make sense if the rectangle is empty
		if(!g.getSize().empty())
			paint(g);

		// restore old rectangle
		g.setMinOff(goff);
		g.setSize(gs);
	}

	void UIElement::repaintRect(const Pos &pos,const Size &size,bool update) {
		if(_g && isDirty()) {
			Window *win;
			if(_parent && (win = getWindow()) && !win->_repainting)
				win->repaintRect(getWindowPos() + pos,size,update);
			else {
				paintRect(*_g,pos,size);
				debug();
				if(update)
					_g->requestUpdate();
			}
			makeClean();
		}
	}

	void UIElement::makeClean() {
		if(getGraphics()->getBuffer()->isReady()) {
			_dirty = false;
			_theme.setDirty(false);
		}
	}

	void UIElement::print(esc::OStream &os, bool, size_t indent) const {
		os << esc::fmt("",indent) << "[" << typeid(*this).name() << "]";
		os << " id=" << getId() << " winpos=" << getWindowPos() << " pos=" << getPos()
				<< " size=" << getSize();
	}
}
