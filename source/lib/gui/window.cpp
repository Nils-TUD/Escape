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
#include <gui/graphics/graphicfactory.h>
#include <gui/window.h>
#include <gui/uielement.h>
#include <gui/imagebutton.h>
#include <esc/messages.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/io.h>
#include <string>
#include <ostream>
#include <iomanip>

using namespace std;

namespace gui {
	const char *WindowTitleBar::CLOSE_IMG = "/etc/close.bmp";

	WindowTitleBar::WindowTitleBar(const string& title,const Pos &pos,const Size &size)
		: Panel(pos,size), _title(make_control<Label>(title)) {
	}

	void WindowTitleBar::onButtonClick(A_UNUSED UIElement& el) {
		Application::getInstance()->exit();
	}

	void WindowTitleBar::init() {
		setLayout(make_layout<BorderLayout>());
		add(_title,BorderLayout::WEST);
		shared_ptr<Button> btn = make_control<ImageButton>(Image::loadImage(CLOSE_IMG),false);
		btn->clicked().subscribe(mem_recv(this,&WindowTitleBar::onButtonClick));
		add(btn,BorderLayout::EAST);
	}


	gwinid_t Window::NEXT_TMP_ID		= 0xFFFF;
	const gsize_t Window::MIN_WIDTH		= 40;
	const gsize_t Window::MIN_HEIGHT	= 40;
	const gsize_t Window::HEADER_SIZE	= 28;

	Window::Window(const Pos &pos,const Size &size,Style style)
		: UIElement(pos,Size(max(MIN_WIDTH,size.width),max(MIN_HEIGHT,size.height))),
			_id(NEXT_TMP_ID--), _created(false), _style(style),
			_inTitle(false), _inResizeLeft(false), _inResizeRight(false), _inResizeBottom(false),
			_isActive(false), _repainting(false), _movePos(pos), _resizeSize(getSize()),
			_gbuf(nullptr), _header(), _body(make_control<Panel>(Pos(1,1),getSize() - Size(2,2))),
			_tabCtrls(), _tabIt(_tabCtrls.begin()) {
		init();
	}
	Window::Window(const string &title,const Pos &pos,const Size &size,Style style)
		: UIElement(pos,Size(max(MIN_WIDTH,size.width),max(MIN_HEIGHT,size.height))),
			_id(NEXT_TMP_ID--), _created(false), _style(style),
			_inTitle(false), _inResizeLeft(false), _inResizeRight(false), _inResizeBottom(false),
			_isActive(false), _repainting(false), _movePos(pos), _resizeSize(getSize()), _gbuf(nullptr),
			_header(make_control<WindowTitleBar>(title,Pos(1,1),Size(getSize().width - 2,HEADER_SIZE))),
			_body(make_control<Panel>(Pos(1,1 + HEADER_SIZE),getSize() - Size(2,HEADER_SIZE + 2))),
			_tabCtrls(), _tabIt(_tabCtrls.begin()) {
		init();
	}

	Window::~Window() {
		delete _gbuf;
	}

	void Window::init() {
		Application *app = Application::getInstance();
		getTheme().setPadding(0);
		_gbuf = new GraphicsBuffer(this,getPos(),getSize(),app->getColorDepth());
		_g = GraphicFactory::get(_gbuf,getSize());
		gsize_t y = 1;
		gsize_t height = getSize().height - 1;
		if(_header) {
			_header->_g = GraphicFactory::get(_gbuf,Size(getSize().width - 2,_header->getSize().height));
			_header->_g->setOff(Pos(1,1));
			_header->_g->setMinOff(Pos(1,1));
			_header->_parent = this;
			_header->init();
			y += _header->getSize().height;
			height -= _header->getSize().height;
		}
		_body->_g = GraphicFactory::get(_gbuf,Size(getSize().width - 2,height));
		_body->_g->setOff(Pos(1,y));
		_body->_g->setMinOff(Pos(1,y));
		_body->_parent = this;
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
		if(_style == DEFAULT) {
			bool resizing = _inResizeBottom || _inResizeLeft || _inResizeRight;
			bool title = _inTitle;

			_inTitle = false;
			_inResizeBottom = false;
			_inResizeLeft = false;
			_inResizeRight = false;

			// finish resize-operation
			if(_movePos.x != getPos().x || _resizeSize != getSize()) {
				if(resizing) {
					prepareResize(_movePos,_resizeSize);
					_gbuf->setUpdating();
					Application::getInstance()->resizeWindow(this,true);
				}
			}

			// finish move-operation
			if(_movePos != getPos()) {
				if(title) {
					_gbuf->moveTo(_movePos);
					setPos(_movePos);
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
		if(_style == DEFAULT) {
			if(_header && e.getPos().y < (gpos_t)_header->getSize().height)
				_inTitle = true;
			else if(e.getPos().y >= (gpos_t)(getSize().height - CURSOR_RESIZE_WIDTH))
				_inResizeBottom = true;
			if(!_header || e.getPos().y >= (gpos_t)_header->getSize().height) {
				if(e.getPos().x < CURSOR_RESIZE_WIDTH)
					_inResizeLeft = true;
				else if(e.getPos().x >= (gpos_t)(getSize().width - CURSOR_RESIZE_WIDTH))
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

	void Window::prepareResize(const Pos &pos,const Size &size) {
		_gbuf->freeBuffer();
		_gbuf->moveTo(pos);
		_gbuf->resizeTo(size);
		_g->setSize(size);
		// when resizing left, its both a resize and move
		setPos(pos);
		setSize(size);
		if(_header) {
			_header->resizeTo(Size(size.width - 2,_header->getSize().height));
			_body->resizeTo(Size(size.width - 2,size.height - _header->getSize().height - 2));
		}
		else
			_body->resizeTo(Size(size.width - 2,size.height - 2));
		_movePos = pos;
		_resizeSize = size;
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
		switch(event) {
			case KeyEvent::KEY_PRESSED:
				keyPressed().send(*this,e);
				if(focus)
					focus->onKeyPressed(e);
				break;
			case KeyEvent::KEY_RELEASED:
				keyReleased().send(*this,e);
				if(focus)
					focus->onKeyReleased(e);
				break;
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
			passMouseToCtrl(focus,e);
			return;
		}

		if(_header && e.getPos().y < (gpos_t)_header->getSize().height)
			passMouseToCtrl(_header.get(),e);
		else
			passMouseToCtrl(_body.get(),e);

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
		Pos pos = e.getPos() - (c ? c->getWindowPos() : Pos(0,0));
		MouseEvent ce(e.getType(),e.getXMovement(),e.getYMovement(),e.getWheelMovement(),
				pos,e.getButtonMask());
		switch(e.getType()) {
			case MouseEvent::MOUSE_MOVED:
				mouseMoved().send(*this,e);
				if(c)
					c->onMouseMoved(ce);
				break;
			case MouseEvent::MOUSE_PRESSED:
				mousePressed().send(*this,e);
				if(c)
					c->onMousePressed(ce);
				break;
			case MouseEvent::MOUSE_RELEASED:
				mouseReleased().send(*this,e);
				if(c)
					c->onMouseReleased(ce);
				break;
			case MouseEvent::MOUSE_WHEEL:
				mouseWheel().send(*this,e);
				if(c)
					c->onMouseWheel(ce);
				break;
		}
	}

	void Window::requestFocus(Control *c) {
		if(c == nullptr || c->getWindow() == this) {
			Control *old = getFocus();
			if(old != c) {
				if(old)
					old->onFocusLost();
				if(c)
					c->onFocusGained();
			}
		}
	}

	void Window::setFocus(Control *c) {
		auto res = find_if(_body->_controls.begin(),_body->_controls.end(),
			[c] (const shared_ptr<Control> &b) {
				return c == b.get();
			}
		);
		if(res != _body->_controls.end()) {
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

	void Window::resize(int width,int height) {
		if(_resizeSize.width + width < MIN_WIDTH)
			width = -_resizeSize.width + MIN_WIDTH;
		if(_resizeSize.height + height < MIN_HEIGHT)
			height = -_resizeSize.height + MIN_HEIGHT;
		resizeTo(Size(_resizeSize.width + width,_resizeSize.height + height));
	}

	void Window::resizeTo(const Size &size) {
		resizeMoveTo(_movePos.x,size);
	}

	void Window::resizeMove(int x,int width,int height) {
		if(_resizeSize.width + width >= MIN_WIDTH && _resizeSize.height + height >= MIN_HEIGHT)
			resizeMoveTo(_movePos.x + x,Size(_resizeSize.width + width,_resizeSize.height + height));
	}

	void Window::resizeMoveTo(gpos_t x,const Size &size) {
		// no resize until we're created
		if(!_created)
			return;

		Size rsize = minsize(size,Application::getInstance()->getScreenSize());
		if(_movePos.x != x || rsize != _resizeSize) {
			_movePos.x = x;
			_resizeSize = rsize;
			_gbuf->setUpdating();
			Application::getInstance()->resizeWindow(this,false);
		}
	}

	void Window::move(int x,int y) {
		Size screenSize = Application::getInstance()->getScreenSize();
		x = min((gpos_t)screenSize.width - 1,_movePos.x + x);
		y = max(0,min((gpos_t)screenSize.height - 1,_movePos.y + y));
		moveTo(Pos(x,y));
	}

	void Window::moveTo(const Pos &pos) {
		// no move until we're created
		if(!_created)
			return;

		if(_movePos != pos) {
			_movePos = pos;
			Application::getInstance()->moveWindow(this,false);
		}
	}

	void Window::paintTitle(A_UNUSED Graphics &g) {
		// no repaint until we're created and popups have no title-bar
		if(!_created || _style == POPUP || !_header)
			return;

		_header->repaint(false);
	}

	void Window::paint(Graphics &g) {
		// no repaint until we're created
		if(!_created)
			return;

		paintTitle(g);

		// draw border
		g.setColor(getTheme().getColor(Theme::WIN_BORDER));
		g.drawRect(Pos(0,0),getSize());

		if(_repainting) {
			Rectangle inter = intersection(g.getPaintRect(),
					Rectangle(_body->getPos(),_body->getSize()));
			if(!inter.empty())
				_body->repaintRect(inter.getPos() - _body->getPos(),inter.getSize(),false);
		}
		else
			_body->repaint(false);
	}

	void Window::paintRect(Graphics &g,const Pos &pos,const Size &size) {
		_repainting = true;
		UIElement::paintRect(g,pos,size);
		_repainting = false;
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

	void Window::onCreated(gwinid_t wid) {
		_id = wid;
		_created = true;
		_gbuf->allocBuffer();
		repaint();
	}

	void Window::onResized() {
		_gbuf->allocBuffer();
		_gbuf->onUpdated();
		// force a complete repaint
		if(_header)
			_header->makeDirty(true);
		_body->makeDirty(true);
		repaint();
	}

	void Window::onUpdated() {
		_gbuf->onUpdated();
	}

	void Window::print(std::ostream &os, bool rec, size_t indent) const {
		UIElement::print(os,rec,indent);
		if(rec) {
			os << " {\n";
			_header->print(os,rec,indent + 2);
			os << "\n";
			_body->print(os,rec,indent + 2);
			os << "\n" << std::setw(indent) << "" << "}";
		}
	}
}
