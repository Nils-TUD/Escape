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
#include <gui/window.h>
#include <gui/uielement.h>
#include <gui/color.h>
#include <gui/graphicfactory.h>
#include <esc/messages.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/io.h>
#include <string>
#include <ostream>

namespace gui {
	Color Window::BGCOLOR = Color(0x88,0x88,0x88);
	Color Window::TITLE_ACTIVE_BGCOLOR = Color(0,0,0xFF);
	Color Window::TITLE_INACTIVE_BGCOLOR = Color(0,0,0x80);
	Color Window::TITLE_FGCOLOR = Color(0xFF,0xFF,0xFF);
	Color Window::BORDER_COLOR = Color(0x77,0x77,0x77);

	tWinId Window::NEXT_TMP_ID = 0xFFFF;

	Window::Window(const string &title,tCoord x,tCoord y,tSize width,tSize height,uchar style)
		: UIElement(x,y,MAX(MIN_WIDTH,width),MAX(MIN_HEIGHT,height)),
			_id(NEXT_TMP_ID--), _created(false), _style(style),
			_title(title), _titleBarHeight(20), _inTitle(false), _inResizeLeft(false),
			_inResizeRight(false), _inResizeBottom(false), _isActive(false), _moveX(x), _moveY(y),
			_resizeWidth(width), _resizeHeight(height), _focus(-1), _controls(vector<Control*>()) {
		init();
	}

	Window::Window(const Window &w)
		: UIElement(w), _id(NEXT_TMP_ID--), _created(false), _style(w._style), _title(w._title),
			_titleBarHeight(w._titleBarHeight), _inTitle(w._inTitle), _inResizeLeft(false),
			_inResizeRight(false), _inResizeBottom(false), _isActive(false), _moveX(0),
			_moveY(0), _resizeWidth(0), _resizeHeight(0), _focus(w._focus), _controls(w._controls) {
		init();
	}

	Window::~Window() {
		// no delete of _g here, since UIElement does it for us
		// remove us from app
		Application::getInstance()->removeWindow(this);
	}

	Window &Window::operator=(const Window &w) {
		UIElement::operator=(w);
		_title = w._title;
		_style = w._style;
		_titleBarHeight = w._titleBarHeight;
		_inTitle = w._inTitle;
		_inResizeLeft = w._inResizeLeft;
		_inResizeRight = w._inResizeRight;
		_inResizeBottom = w._inResizeBottom;
		_isActive = false;
		_moveX = w._moveX;
		_moveY = w._moveY;
		_resizeWidth = w._resizeWidth;
		_resizeHeight = w._resizeHeight;
		_focus = w._focus;
		_controls = w._controls;
		_id = NEXT_TMP_ID--;
		_created = false;
		init();
		return *this;
	}

	void Window::init() {
		_g = GraphicFactory::get(getX(),getY(),getWidth(),getHeight(),
				Application::getInstance()->getColorDepth());
		// add us to app; we'll receive a "created"-event as soon as the window
		// manager knows about us
		Application::getInstance()->addWindow(this);
	}

	void Window::onMouseMoved(const MouseEvent &e) {
		// we store on release/pressed whether we are in the header because
		// the delay between window-movement and cursor-movement may be too
		// big so that we "loose" the window
		if(_inTitle) {
			if(e.isButton1Down())
				move(e.getXMovement(),e.getYMovement());
			return;
		}
		if(_inResizeLeft) {
			if(e.isButton1Down()) {
				short width = _inResizeLeft ? -e.getXMovement() : 0;
				short height = _inResizeBottom ? e.getYMovement() : 0;
				resizeMove(e.getXMovement(),width,height);
			}
			return;
		}
		if(_inResizeRight || _inResizeBottom) {
			if(e.isButton1Down())
				resize(_inResizeRight ? e.getXMovement() : 0,_inResizeBottom ? e.getYMovement() : 0);
			return;
		}
		passToCtrl(e,MOUSE_MOVED);
	}

	void Window::onMouseReleased(const MouseEvent &e) {
		if(_style != STYLE_POPUP) {
			bool resizing = _inResizeBottom || _inResizeLeft || _inResizeRight;
			bool title = _inTitle;

			_inTitle = false;
			_inResizeBottom = false;
			_inResizeLeft = false;
			_inResizeRight = false;

			// finish resize-operation
			if(_moveX != getX() || _resizeWidth != getWidth() || _resizeHeight != getHeight()) {
				if(resizing) {
					_g->moveTo(_moveX,_moveY);
					_g->resizeTo(_resizeWidth,_resizeHeight);
					for(vector<Control*>::iterator it = _controls.begin(); it != _controls.end(); ++it)
						(*it)->getGraphics()->resizeTo(_resizeWidth,_resizeHeight);
					// when resizing left, its both a resize and move
					setX(_moveX);
					setY(_moveY);
					setWidth(_resizeWidth);
					setHeight(_resizeHeight);
					Application::getInstance()->resizeWindow(this,true);
					repaint();
				}
			}

			// finish move-operation
			if(_moveX != getX() || _moveY != getY()) {
				_g->moveTo(_moveX,_moveY);
				setX(_g->_x);
				setY(_g->_y);
				Application::getInstance()->moveWindow(this,true);
			}

			// don't pass the event to controls in this case
			if(title || resizing)
				return;
		}
		passToCtrl(e,MOUSE_RELEASED);
	}

	void Window::onMousePressed(const MouseEvent &e) {
		if(_style != STYLE_POPUP) {
			if(e.getY() < _titleBarHeight) {
				_inTitle = true;
				return;
			}
			else if(e.getY() >= getHeight() - CURSOR_RESIZE_WIDTH)
				_inResizeBottom = true;
			if(e.getX() < CURSOR_RESIZE_WIDTH)
				_inResizeLeft = true;
			else if(e.getX() >= getWidth() - CURSOR_RESIZE_WIDTH)
				_inResizeRight = true;
		}
		passToCtrl(e,MOUSE_PRESSED);
	}

	void Window::onKeyPressed(const KeyEvent &e) {
		passToCtrl(e,KEY_PRESSED);
	}

	void Window::onKeyReleased(const KeyEvent &e) {
		passToCtrl(e,KEY_RELEASED);
	}

	void Window::passToCtrl(const KeyEvent &e,uchar event) {
		// no events until we're created
		if(!_created)
			return;

		// handle focus-change
		if(event == KEY_RELEASED && e.getKeyCode() == VK_TAB) {
			int old = _focus;
			if(_focus == -1)
				_focus = 0;
			else if(e.isShiftDown())
				_focus = (_focus - 1) % _controls.size();
			else
				_focus = (_focus + 1) % _controls.size();

			if(old != _focus) {
				if(old >= 0)
					_controls[old]->onFocusLost();
				_controls[_focus]->onFocusGained();
			}
			return;
		}

		// pass event to focused control
		if(_focus != -1) {
			switch(event) {
				case KEY_PRESSED:
					_controls[_focus]->onKeyPressed(e);
					break;
				case KEY_RELEASED:
					_controls[_focus]->onKeyReleased(e);
					break;
			}
		}
	}

	void Window::passToCtrl(const MouseEvent &e,uchar event) {
		// no events until we're created
		if(!_created)
			return;

		/* if a control is focused and we get a moved or released event we have to pass this
		 * event to the focused control. otherwise the control wouldn't know of them */
		if(event == MOUSE_MOVED) {
			if(_focus >= 0)
				_controls[_focus]->onMouseMoved(e);
			return;
		}
		else if(event == MOUSE_RELEASED) {
			if(_focus >= 0)
				_controls[_focus]->onMouseReleased(e);
			return;
		}

		tCoord x = e.getX();
		tCoord y = e.getY();
		y -= _titleBarHeight;
		for(vector<Control*>::iterator it = _controls.begin(); it != _controls.end(); ++it) {
			Control *c = *it;
			if(x >= c->getX() && x < c->getX() + c->getWidth() &&
				y >= c->getY() && y < c->getY() + c->getHeight()) {
				/* change focus */
				if(_focus < 0 || c != _controls[_focus]) {
					if(_focus >= 0)
						_controls[_focus]->onFocusLost();
					c->onFocusGained();
					_focus = distance(_controls.begin(),it);
				}
				c->onMousePressed(e);
				return;
			}
		}

		// if we're here the user has not clicked on a control, so set the focus to "nothing"
		if(_focus >= 0)
			_controls[_focus]->onFocusLost();
		_focus = -1;
	}

	void Window::resize(short width,short height) {
		if(_resizeWidth + width < MIN_WIDTH)
			width = -_resizeWidth + MIN_WIDTH;
		if(_resizeHeight + height < MIN_HEIGHT)
			height = -_resizeHeight + MIN_HEIGHT;
		resizeTo(_resizeWidth + width,_resizeHeight + height);
	}

	void Window::resizeTo(tSize width,tSize height) {
		resizeMoveTo(_moveX,width,height);
	}

	void Window::resizeMove(short x,short width,short height) {
		if(_resizeWidth + width >= MIN_WIDTH && _resizeHeight + height >= MIN_HEIGHT)
			resizeMoveTo(_moveX + x,_resizeWidth + width,_resizeHeight + height);
	}

	void Window::resizeMoveTo(tCoord x,tSize width,tSize height) {
		// no resize until we're created
		if(!_created)
			return;

		tSize screenWidth = Application::getInstance()->getScreenWidth();
		tSize screenHeight = Application::getInstance()->getScreenHeight();
		width = MIN(screenWidth,width);
		height = MIN(screenHeight,height);
		if(_moveX != x || width != _resizeWidth || height != _resizeHeight) {
			_moveX = x;
			_resizeWidth = width;
			_resizeHeight = height;
			Application::getInstance()->resizeWindow(this,false);
		}
	}

	void Window::move(short x,short y) {
		tSize screenWidth = Application::getInstance()->getScreenWidth();
		tSize screenHeight = Application::getInstance()->getScreenHeight();
		x = MIN(screenWidth - 1,_moveX + x);
		y = MAX(0,MIN(screenHeight - 1,_moveY + y));
		moveTo(x,y);
	}

	void Window::moveTo(tCoord x,tCoord y) {
		// no move until we're created
		if(!_created)
			return;

		if(_moveX != x || _moveY != y) {
			_moveX = x;
			_moveY = y;
			Application::getInstance()->moveWindow(this,false);
		}
	}

	void Window::paintTitle(Graphics &g) {
		// no repaint until we're created and popups have no title-bar
		if(!_created || _style == STYLE_POPUP)
			return;

		// paint titlebar
		if(_isActive)
			g.setColor(TITLE_ACTIVE_BGCOLOR);
		else
			g.setColor(TITLE_INACTIVE_BGCOLOR);
		g.fillRect(1,1,getWidth() - 2,_titleBarHeight - 1);
		g.setColor(TITLE_FGCOLOR);
		g.drawString(5,(_titleBarHeight - g.getFont().getHeight()) / 2,_title);

		// draw cross
		g.setColor(BORDER_COLOR);
		const tSize boxPad = 2;
		const tSize crossPad = 2;
		tSize cboxSize = _titleBarHeight - boxPad * 2;
		g.drawRect(getWidth() - cboxSize - boxPad,boxPad,cboxSize,cboxSize);
		g.drawLine(getWidth() - cboxSize - boxPad + crossPad,boxPad + crossPad,
				getWidth() - boxPad - crossPad,_titleBarHeight - boxPad - crossPad);
		g.drawLine(getWidth() - boxPad - crossPad,boxPad + crossPad,
				getWidth() - cboxSize - boxPad + crossPad,_titleBarHeight - boxPad - crossPad);
	}

	void Window::paint(Graphics &g) {
		// no repaint until we're created
		if(!_created)
			return;

		paintTitle(g);
		// fill bg
		g.setColor(BGCOLOR);
		g.fillRect(0,_titleBarHeight,getWidth(),getHeight());

		// draw border
		g.setColor(BORDER_COLOR);
		g.drawLine(0,_titleBarHeight,getWidth(),_titleBarHeight);
		g.drawRect(0,0,getWidth(),getHeight());

		// first, focus a control, if not already done
		if(_focus == -1 && _controls.size() > 0) {
			_focus = 0;
			_controls[_focus]->onFocusGained();
		}

		// now paint controls
		for(vector<Control*>::iterator it = _controls.begin(); it != _controls.end(); ++it)
			(*it)->repaint();
	}

	void Window::update(tCoord x,tCoord y,tSize width,tSize height) {
		if(_g)
			_g->update(x,y,width,height);
	}

	void Window::add(Control &c) {
		_controls.push_back(&c);
		c.setWindow(this);
	}

	void Window::setActive(bool active) {
		if(active != _isActive) {
			_isActive = active;
			paintTitle(*_g);
			requestUpdate();
		}
	}

	void Window::onCreated(tWinId id) {
		_id = id;
		_created = true;
		repaint();
	}

	ostream &operator<<(ostream &s,const Window &w) {
		string title = w.getTitle();
		s << "Window[id=" << w.getId() << " @" << w.getX() << "," << w.getY();
		s << " size=" << w.getWidth() << ",";
		s << w.getHeight() << " title=" << title << "]";
		return s;
	}
}
