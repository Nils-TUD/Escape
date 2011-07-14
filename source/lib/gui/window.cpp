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
#include <gui/window.h>
#include <gui/uielement.h>
#include <gui/color.h>
#include <gui/graphicfactory.h>
#include <gui/imagebutton.h>
#include <esc/messages.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/io.h>
#include <string>
#include <ostream>

namespace gui {
	const char *WindowTitleBar::CLOSE_IMG = "/etc/close.bmp";

	WindowTitleBar::WindowTitleBar(const string& title,gpos_t x,gpos_t y,
			gsize_t width,gsize_t height)
		: Panel(x,y,width,height), ActionListener(), _title(title), _blayout(NULL) {
	}
	WindowTitleBar::~WindowTitleBar() {
		delete _blayout;
		delete _imgs[0];
		delete _btns[0];
	}

	void WindowTitleBar::init() {
		_blayout = new BorderLayout();
		setLayout(_blayout);
		_imgs[0] = Image::loadImage(CLOSE_IMG);
		_btns[0] = new ImageButton(_imgs[0],false);
		_btns[0]->addListener(this);
		add(*_btns[0],BorderLayout::EAST);
	}

	void WindowTitleBar::actionPerformed(UIElement& el) {
		UNUSED(el);
		Application::getInstance()->exit();
	}

	void WindowTitleBar::paint(Graphics& g) {
		Panel::paint(g);

		g.setColor(getTheme().getColor(Theme::CTRL_FOREGROUND));
		g.drawString(5,(getHeight() - g.getFont().getHeight()) / 2,_title);
	}


	gwinid_t Window::NEXT_TMP_ID = 0xFFFF;

	Window::Window(gpos_t x,gpos_t y,gsize_t width,gsize_t height,uchar style)
		: UIElement(x,y,MAX(MIN_WIDTH,width),MAX(MIN_HEIGHT,height)),
			_id(NEXT_TMP_ID--), _created(false), _style(style),
			_inTitle(false), _inResizeLeft(false), _inResizeRight(false), _inResizeBottom(false),
			_isActive(false), _moveX(x), _moveY(y), _resizeWidth(getWidth()), _resizeHeight(getHeight()),
			_gbuf(NULL), _header(NULL),
			_body(Panel(1,1,getWidth() - 2,getHeight() - 2)),
			_tabCtrls(list<Control*>()), _tabIt(_tabCtrls.begin()) {
		init();
	}
	Window::Window(const string &title,gpos_t x,gpos_t y,gsize_t width,gsize_t height,uchar style)
		: UIElement(x,y,MAX(MIN_WIDTH,width),MAX(MIN_HEIGHT,height)),
			_id(NEXT_TMP_ID--), _created(false), _style(style),
			_inTitle(false), _inResizeLeft(false), _inResizeRight(false), _inResizeBottom(false),
			_isActive(false), _moveX(x), _moveY(y), _resizeWidth(getWidth()), _resizeHeight(getHeight()),
			_gbuf(NULL), _header(new WindowTitleBar(title,1,1,getWidth() - 2,HEADER_SIZE)),
			_body(Panel(1,1 + HEADER_SIZE,getWidth() - 2,getHeight() - HEADER_SIZE - 2)),
			_tabCtrls(list<Control*>()), _tabIt(_tabCtrls.begin()) {
		init();
	}

	Window::~Window() {
		// remove us from app
		Application::getInstance()->removeWindow(this);
		delete _header;
		delete _gbuf;
	}

	void Window::init() {
		Application *app = Application::getInstance();
		getTheme().setPadding(0);
		_gbuf = new GraphicsBuffer(this,getX(),getY(),getWidth(),getHeight(),app->getColorDepth());
		_g = GraphicFactory::get(_gbuf,getWidth(),getHeight());
		// add us to app; we'll receive a "created"-event as soon as the window
		// manager knows about us
		app->addWindow(this);
		gsize_t y = 1;
		gsize_t height = getHeight() - 1;
		if(_header) {
			_header->_g = GraphicFactory::get(_gbuf,getWidth() - 2,_header->getHeight());
			_header->_g->setOff(1,1);
			_header->_g->setMinOff(1,1);
			_header->_parent = this;
			_header->init();
			y += _header->getHeight();
			height -= _header->getHeight();
		}
		_body._g = GraphicFactory::get(_gbuf,getWidth() - 2,height);
		_body._g->setOff(1,y);
		_body._g->setMinOff(1,y);
		_body._parent = this;
	}

	void Window::onMouseMoved(const MouseEvent &e) {
		// we store on release/pressed whether we are in the header because
		// the delay between window-movement and cursor-movement may be too
		// big so that we "loose" the window
		if(_inTitle) {
			if(e.isButton1Down()) {
				move(e.getXMovement(),e.getYMovement());
				return;
			}
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
		passToCtrl(e,MouseEvent::MOUSE_MOVED);
	}

	void Window::onMouseReleased(const MouseEvent &e) {
		if(_style == STYLE_DEFAULT) {
			bool resizing = _inResizeBottom || _inResizeLeft || _inResizeRight;
			bool title = _inTitle;

			_inTitle = false;
			_inResizeBottom = false;
			_inResizeLeft = false;
			_inResizeRight = false;

			// finish resize-operation
			if(_moveX != getX() || _resizeWidth != getWidth() || _resizeHeight != getHeight()) {
				if(resizing) {
					_gbuf->moveTo(_moveX,_moveY);
					_gbuf->resizeTo(_resizeWidth,_resizeHeight);
					_g->setSize(0,0,_resizeWidth,_resizeHeight,_resizeWidth,_resizeHeight);
					// when resizing left, its both a resize and move
					setX(_moveX);
					setY(_moveY);
					setWidth(_resizeWidth);
					setHeight(_resizeHeight);
					if(_header) {
						_header->resizeTo(_resizeWidth - 2,_header->getHeight());
						_body.resizeTo(_resizeWidth - 2,_resizeHeight - _header->getHeight() - 2);
					}
					else
						_body.resizeTo(_resizeWidth - 2,_resizeHeight - 2);
					Application::getInstance()->resizeWindow(this,true);
					repaint();
				}
			}

			// finish move-operation
			if(_moveX != getX() || _moveY != getY()) {
				if(title) {
					_gbuf->moveTo(_moveX,_moveY);
					setX(_moveX);
					setY(_moveY);
					Application::getInstance()->moveWindow(this,true);
				}
			}

			// don't pass the event to controls in this case
			if(title || resizing)
				return;
		}
		passToCtrl(e,MouseEvent::MOUSE_RELEASED);
	}

	void Window::onMousePressed(const MouseEvent &e) {
		if(_style == STYLE_DEFAULT) {
			if(_header && e.getY() < _header->getHeight())
				_inTitle = true;
			else if(e.getY() >= getHeight() - CURSOR_RESIZE_WIDTH)
				_inResizeBottom = true;
			if(!_header || e.getY() >= _header->getHeight()) {
				if(e.getX() < CURSOR_RESIZE_WIDTH)
					_inResizeLeft = true;
				else if(e.getX() >= getWidth() - CURSOR_RESIZE_WIDTH)
					_inResizeRight = true;
			}
			// don't pass the event to a control
			if(_inResizeBottom || _inResizeLeft || _inResizeRight)
				return;
		}
		passToCtrl(e,MouseEvent::MOUSE_PRESSED);
	}

	void Window::onMouseWheel(const MouseEvent &e) {
		passToCtrl(e,MouseEvent::MOUSE_WHEEL);
	}

	void Window::onKeyPressed(const KeyEvent &e) {
		passToCtrl(e,KeyEvent::KEY_PRESSED);
	}

	void Window::onKeyReleased(const KeyEvent &e) {
		passToCtrl(e,KeyEvent::KEY_RELEASED);
	}

	void Window::passToCtrl(const KeyEvent &e,uchar event) {
		// no events until we're created
		if(!_created)
			return;

		// handle focus-change
		if(event == KeyEvent::KEY_RELEASED && e.getKeyCode() == VK_TAB) {
			if(_tabCtrls.size() > 0) {
				Control *oldFocus = getFocus();
				if(_tabIt == _tabCtrls.end())
					_tabIt = _tabCtrls.begin();

				if(e.isShiftDown()) {
					if(_tabIt == _tabCtrls.begin())
						_tabIt = _tabCtrls.end();
					_tabIt--;
				}
				else {
					_tabIt++;
					if(_tabIt == _tabCtrls.end())
						_tabIt = _tabCtrls.begin();
				}

				if(*_tabIt != oldFocus) {
					if(oldFocus)
						oldFocus->onFocusLost();
					(*_tabIt)->onFocusGained();
				}
			}
			return;
		}

		// pass event to focused control
		Control *focus = getFocus();
		if(focus) {
			switch(event) {
				case KeyEvent::KEY_PRESSED:
					focus->onKeyPressed(e);
					break;
				case KeyEvent::KEY_RELEASED:
					focus->onKeyReleased(e);
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
		Control *focus = getFocus();
		if(event == MouseEvent::MOUSE_MOVED || event == MouseEvent::MOUSE_RELEASED) {
			if(focus)
				passMouseToCtrl(focus,e);
			return;
		}

		if(_header && e.getY() < _header->getHeight())
			passMouseToCtrl(_header,e);
		else
			passMouseToCtrl(&_body,e);

		// handle focus-change
		Control *newFocus = getFocus();
		if(newFocus != focus) {
			if(focus)
				focus->onFocusLost();
			if(newFocus)
				newFocus->onFocusGained();
		}
	}

	void Window::passMouseToCtrl(Control *c,const MouseEvent& e) {
		gpos_t x = e.getX() - c->getWindowX();
		gpos_t y = e.getY() - c->getWindowY();
		MouseEvent ce(e.getType(),e.getXMovement(),e.getYMovement(),e.getWheelMovement(),
				x,y,e.getButtonMask());
		switch(e.getType()) {
			case MouseEvent::MOUSE_MOVED:
				c->onMouseMoved(ce);
				break;
			case MouseEvent::MOUSE_PRESSED:
				c->onMousePressed(ce);
				break;
			case MouseEvent::MOUSE_RELEASED:
				c->onMouseReleased(ce);
				break;
			case MouseEvent::MOUSE_WHEEL:
				c->onMouseWheel(ce);
				break;
		}
	}

	void Window::setFocus(Control *c) {
		if(find(_body._controls.begin(),_body._controls.end(),c) != _body._controls.end()) {
			Control *old = getFocus();
			if(old != c) {
				if(old)
					old->onFocusLost();
				if(c)
					c->onFocusGained();
			}
		}
	}

	void Window::appendTabCtrl(Control &c) {
		_tabCtrls.push_back(&c);
		_tabIt = _tabCtrls.end();
	}

	void Window::resize(short width,short height) {
		if(_resizeWidth + width < MIN_WIDTH)
			width = -_resizeWidth + MIN_WIDTH;
		if(_resizeHeight + height < MIN_HEIGHT)
			height = -_resizeHeight + MIN_HEIGHT;
		resizeTo(_resizeWidth + width,_resizeHeight + height);
	}

	void Window::resizeTo(gsize_t width,gsize_t height) {
		resizeMoveTo(_moveX,width,height);
	}

	void Window::resizeMove(short x,short width,short height) {
		if(_resizeWidth + width >= MIN_WIDTH && _resizeHeight + height >= MIN_HEIGHT)
			resizeMoveTo(_moveX + x,_resizeWidth + width,_resizeHeight + height);
	}

	void Window::resizeMoveTo(gpos_t x,gsize_t width,gsize_t height) {
		// no resize until we're created
		if(!_created)
			return;

		gsize_t screenWidth = Application::getInstance()->getScreenWidth();
		gsize_t screenHeight = Application::getInstance()->getScreenHeight();
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
		gsize_t screenWidth = Application::getInstance()->getScreenWidth();
		gsize_t screenHeight = Application::getInstance()->getScreenHeight();
		x = MIN(screenWidth - 1,_moveX + x);
		y = MAX(0,MIN(screenHeight - 1,_moveY + y));
		moveTo(x,y);
	}

	void Window::moveTo(gpos_t x,gpos_t y) {
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
		UNUSED(g);
		// no repaint until we're created and popups have no title-bar
		if(!_created || _style == STYLE_POPUP || !_header)
			return;

		_header->paint(*_header->getGraphics());
	}

	void Window::paint(Graphics &g) {
		// no repaint until we're created
		if(!_created)
			return;

		paintTitle(g);

		// draw border
		g.setColor(getTheme().getColor(Theme::WIN_BORDER));
		g.drawRect(0,0,getWidth(),getHeight());

		_body.paint(*_body.getGraphics());
	}

	void Window::update(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		if(_g)
			_g->update(x,y,width,height);
	}

	void Window::updateActive(bool active) {
		if(active != _isActive) {
			_isActive = active;
			if(_header) {
				Theme::colid_type bgid,fgid;
				if(active) {
					bgid = Theme::WIN_TITLE_ACT_BG;
					fgid = Theme::WIN_TITLE_ACT_FG;
				}
				else {
					bgid = Theme::WIN_TITLE_INACT_BG;
					fgid = Theme::WIN_TITLE_INACT_FG;
				}
				const Color &bg = getTheme().getColor(bgid);
				const Color &fg = getTheme().getColor(fgid);
				_header->getTheme().setColor(Theme::CTRL_BACKGROUND,bg);
				_header->getTheme().setColor(Theme::CTRL_FOREGROUND,fg);
				_header->repaint();
			}
		}
	}

	void Window::onCreated(gwinid_t id) {
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
