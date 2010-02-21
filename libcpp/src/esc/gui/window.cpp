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
#include <esc/io.h>
#include <esc/debug.h>
#include <messages.h>
#include <esc/proc.h>
#include <esc/gui/window.h>
#include <esc/gui/uielement.h>
#include <esc/gui/color.h>
#include <esc/gui/graphicfactory.h>
#include <esc/string.h>
#include <stdlib.h>

namespace esc {
	namespace gui {
		Color Window::BGCOLOR = Color(0x88,0x88,0x88);
		Color Window::TITLE_BGCOLOR = Color(0,0,0xFF);
		Color Window::TITLE_ACTIVE_BGCOLOR = Color(0,0,0x80);
		Color Window::TITLE_FGCOLOR = Color(0xFF,0xFF,0xFF);
		Color Window::BORDER_COLOR = Color(0x77,0x77,0x77);

		tWinId Window::NEXT_TMP_ID = 0xFFFF;

		Window::Window(const String &title,tCoord x,tCoord y,tSize width,tSize height,u8 style)
			: UIElement(x,y,MAX(MIN_WIDTH,width),MAX(MIN_HEIGHT,height)),
				_id(NEXT_TMP_ID--), _created(false), _style(style),
				_title(title), _titleBarHeight(20), _inTitle(false), _inResizeLeft(false),
				_inResizeRight(false), _inResizeBottom(false), _isActive(false), _focus(-1),
				_controls(Vector<Control*>()) {
			init();
		}

		Window::Window(const Window &w)
			: UIElement(w), _id(NEXT_TMP_ID--), _created(false), _style(w._style), _title(w._title),
				_titleBarHeight(w._titleBarHeight), _inTitle(w._inTitle), _inResizeLeft(false),
				_inResizeRight(false), _inResizeBottom(false), _isActive(false),
				_focus(w._focus), _controls(w._controls) {
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
			// we store on release/pressed wether we are in the header because
			// the delay between window-movement and cursor-movement may be too
			// big so that we "loose" the window
			if(_inTitle) {
				if(e.isButton1Down())
					move(e.getXMovement(),e.getYMovement());
				return;
			}
			if(_inResizeLeft) {
				if(e.isButton1Down()) {
					if(_inResizeLeft)
						move(e.getXMovement(),0);
					resize(_inResizeLeft ? -e.getXMovement() : 0,_inResizeBottom ? e.getYMovement() : 0);
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
				if(e.getY() < _titleBarHeight) {
					_inTitle = false;
					return;
				}
				else if(e.getY() >= getHeight() - CURSOR_RESIZE_WIDTH)
					_inResizeBottom = false;
				if(e.getX() < CURSOR_RESIZE_WIDTH)
					_inResizeLeft = false;
				else if(e.getX() >= getWidth() - CURSOR_RESIZE_WIDTH)
					_inResizeRight = false;
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

		void Window::passToCtrl(const KeyEvent &e,u8 event) {
			// no events until we're created
			if(!_created)
				return;

			// handle focus-change
			if(event == KEY_RELEASED && e.getKeyCode() == VK_TAB) {
				s32 old = _focus;
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

		void Window::passToCtrl(const MouseEvent &e,u8 event) {
			// no events until we're created
			if(!_created)
				return;

			Control *c;
			tCoord x = e.getX();
			tCoord y = e.getY();
			y -= _titleBarHeight;
			for(u32 i = 0; i < _controls.size(); i++) {
				c = _controls[i];
				if(x >= c->getX() && x < c->getX() + c->getWidth() &&
					y >= c->getY() && y < c->getY() + c->getHeight()) {
					switch(event) {
						case MOUSE_MOVED:
							c->onMouseMoved(e);
							break;
						case MOUSE_RELEASED:
							/* change focus */
							if((s32)i != _focus) {
								if(_focus >= 0)
									_controls[_focus]->onFocusLost();
								_controls[i]->onFocusGained();
								_focus = i;
							}

							c->onMouseReleased(e);
							break;
						case MOUSE_PRESSED:
							c->onMousePressed(e);
							break;
					}
					break;
				}
			}
		}

		void Window::resize(s16 width,s16 height) {
			if(getWidth() + width < MIN_WIDTH)
				width = -getWidth() + MIN_WIDTH;
			if(getHeight() + height < MIN_HEIGHT)
				height = -getHeight() + MIN_HEIGHT;
			resizeTo(getWidth() + width,getHeight() + height);
		}

		void Window::resizeTo(tSize width,tSize height) {
			tSize screenWidth = Application::getInstance()->getScreenWidth();
			tSize screenHeight = Application::getInstance()->getScreenHeight();
			width = MIN(screenWidth,width);
			height = MIN(screenHeight,height);
			if(width != getWidth() || height != getHeight()) {
				_g->resizeTo(width,height);
				for(u32 i = 0; i < _controls.size(); i++)
					_controls[i]->getGraphics()->resizeTo(width,height);
				setWidth(width);
				setHeight(height);
				Application::getInstance()->resizeWindow(this);
				repaint();
			}
		}

		void Window::move(s16 x,s16 y) {
			tSize screenWidth = Application::getInstance()->getScreenWidth();
			tSize screenHeight = Application::getInstance()->getScreenHeight();
			x = MAX(-getWidth(),MIN(screenWidth - 1,getX() + x));
			y = MAX(0,MIN(screenHeight - 1,getY() + y));
			moveTo(x,y);
		}

		void Window::moveTo(tCoord x,tCoord y) {
			// no move until we're created
			if(!_created)
				return;

			if(getX() != x || getY() != y) {
				_g->moveTo(x,y);
				setX(_g->_x);
				setY(_g->_y);
				Application::getInstance()->moveWindow(this);
			}
		}

		void Window::paintTitle(Graphics &g) {
			// no repaint until we're created and popups have no title-bar
			if(!_created || _style == STYLE_POPUP)
				return;

			// paint titlebar
			if(_isActive)
				g.setColor(TITLE_BGCOLOR);
			else
				g.setColor(TITLE_ACTIVE_BGCOLOR);
			g.fillRect(1,1,getWidth() - 2,_titleBarHeight - 1);
			g.setColor(TITLE_FGCOLOR);
			g.drawString(5,(_titleBarHeight - g.getFont().getHeight()) / 2,_title);

			// draw cross
			g.setColor(BORDER_COLOR);
			const u32 boxPad = 2;
			const u32 crossPad = 2;
			u32 cboxSize = _titleBarHeight - boxPad * 2;
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
			for(u32 i = 0; i < _controls.size(); i++)
				_controls[i]->repaint();
		}

		void Window::update(tCoord x,tCoord y,tSize width,tSize height) {
			if(_g)
				_g->update(x,y,width,height);
		}

		void Window::add(Control &c) {
			_controls.add(&c);
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

		Stream &operator<<(Stream &s,const Window &w) {
			String title = w.getTitle();
			s << "Window[id=" << w.getId() << " @" << w.getX() << "," << w.getY();
			s << " size=" << w.getWidth() << ",";
			s << w.getHeight() << " title=" << title << "]";
			return s;
		}
	}
}
