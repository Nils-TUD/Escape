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

#pragma once

#include <esc/common.h>
#include <esc/io.h>
#include <esc/messages.h>
#include <gui/graphics/graphicsbuffer.h>
#include <gui/graphics/color.h>
#include <gui/layout/borderlayout.h>
#include <gui/image/image.h>
#include <gui/uielement.h>
#include <gui/application.h>
#include <gui/button.h>
#include <gui/control.h>
#include <gui/panel.h>
#include <gui/label.h>
#include <string>
#include <ostream>
#include <list>

namespace gui {
	class WindowTitleBar : public Panel {
		friend class Window;

	private:
		static const char *CLOSE_IMG;

	public:
		WindowTitleBar(const std::string& title,const Pos &pos,const Size &size);

		/**
		 * @return the title (a copy)
		 */
		const std::string &getTitle() const {
			return _title->getText();
		}
		/**
		 * Sets the title and requests a repaint
		 *
		 * @param title the new title
		 */
		void setTitle(const std::string &title) {
			_title->setText(title);
		}

	private:
		void init();
		void onButtonClick(UIElement& el);

		std::shared_ptr<Label> _title;
	};

	/**
	 * Represents a window in the GUI.
	 */
	class Window : public UIElement {
		friend class Application;
		friend class Panel;

	private:
		// used for creation
		static gwinid_t NEXT_TMP_ID;

		// minimum window-size
		static const gsize_t MIN_WIDTH;
		static const gsize_t MIN_HEIGHT;

		static const gsize_t HEADER_SIZE;

	public:
		enum Style {
			/**
			 * For all ordinary windows
			 */
			DEFAULT,
			/**
			 * This is used for popups (e.g. combobox). They are not moveable and resizeable
			 */
			POPUP,
			/**
			 * Only for the desktop-application. Not moveable or resizeable and can't be active.
			 */
			DESKTOP
		};

	public:
		/**
		 * Creates a new window without titlebar
		 *
		 * @param title the window-title
		 * @param pos the position
		 * @param size the size
		 * @param style the window-style (STYLE_*)
		 */
		Window(const Pos &pos,const Size &size = Size(MIN_WIDTH,MIN_HEIGHT),Style style = DEFAULT);
		/**
		 * Creates a new window with titlebar, that displays <title>
		 *
		 * @param title the window-title
		 * @param pos the position
		 * @param size the size
		 * @param style the window-style (STYLE_*)
		 */
		Window(const std::string &title,const Pos &pos,
		       const Size &size = Size(MIN_WIDTH,MIN_HEIGHT),Style style = DEFAULT);
		/**
		 * Destructor
		 */
		virtual ~Window();

		/**
		 * @return the id of the window
		 */
		gwinid_t getId() const {
			return _id;
		}
		/**
		 * @return the style of the window (STYLE_*)
		 */
		uchar getStyle() const {
			return _style;
		}
		/**
		 * @return true if this is the active window
		 */
		bool isActive() const {
			return _isActive;
		}
		/**
		 * @return true if the creation of the window is finished
		 */
		bool isCreated() const {
			return _created;
		}
		/**
		 * @return whether this window has a title-bar
		 */
		bool hasTitleBar() const {
			return (bool)_header;
		}
		/**
		 * @return the title (a copy)
		 */
		const std::string &getTitle() const {
			if(!_header)
				throw std::logic_error("This window has no title");
			return _header->getTitle();
		}
		/**
		 * Sets the title and requests a repaint
		 *
		 * @param title the new title
		 */
		void setTitle(const std::string &title) {
			if(!_header)
				throw std::logic_error("This window has no title");
			_header->setTitle(title);
		}
		/**
		 * @return the focused control (nullptr if none)
		 */
		Control *getFocus() {
			return _body->getFocus();
		}
		/**
		 * Sets the focus on the given control.
		 *
		 * @param c the control
		 */
		void setFocus(Control *c);
		/**
		 * @return the height of the title-bar
		 */
		gsize_t getTitleBarHeight() const {
			return _header ? _header->getSize().height : 0;
		}

		/**
		 * @return the root-panel to which you can add controls
		 */
		std::shared_ptr<Panel> getRootPanel() {
			return _body;
		}

		virtual Size getPrefSize() const {
			Size bsize = _body->getPreferredSize();
			if(_header) {
				Size hsize = _header->getPreferredSize();
				return Size(std::max(hsize.width,bsize.width) + 2,hsize.height + bsize.height + 2);
			}
			return Size(bsize.width + 2,bsize.height + 2);
		}

		virtual void layout() {
			if(_header)
				_header->layout();
			_body->layout();
		}

		/**
		 * Calculates the layout and finally shows the window, i.e. this method has to be called to
		 * make the window visible.
		 *
		 * @param pack whether to pack the window to the preferred required size for the content
		 */
		void show(bool pack = false) {
			if(pack) {
				// overwrite preferred size
				_body->_prefSize = Size();
				if(_header)
					_header->_prefSize = Size();
				prepareResize(getPos(),getPrefSize());
			}
			layout();
		}

		/**
		 * The event-callbacks
		 *
		 * @param e the event
		 */
		virtual void onMouseMoved(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseWheel(const MouseEvent &e);
		virtual void onKeyPressed(const KeyEvent &e);
		virtual void onKeyReleased(const KeyEvent &e);

		/**
		 * Sets whether this window is active. Requests a repaint, if necessary.
		 *
		 * @param active the new value
		 */
		void setActive(bool) {
			// TODO what about the bool?
			Application::getInstance()->requestActiveWindow(_id);
		}

		/**
		 * Adds the given control as tab-control to this window
		 *
		 * @param c the control
		 */
		void appendTabCtrl(Control &c);

	protected:
		/**
		 * Paints only the title of the window
		 *
		 * @param g the graphics-object
		 */
		void paintTitle(Graphics &g);
		virtual void paint(Graphics &g);

		/**
		 * @return the window the ui-element belongs to (this)
		 */
		virtual Window *getWindow() {
			return this;
		}
		virtual const Window *getWindow() const {
			return this;
		}

	private:
		/**
		 * @return the current position to move to
		 */
		Pos getMovePos() const {
			return _movePos;
		}
		/**
		 * @return the current size to resize to
		 */
		Size getResizeSize() const {
			return _resizeSize;
		}
		/**
		 * Sets whether this window is active. Requests a repaint, if necessary.
		 *
		 * @param active the new value
		 */
		void updateActive(bool active);
		/**
		 * Called by Application when the window has been created successfully
		 *
		 * @param id the received window-id
		 */
		void onCreated(gwinid_t id);
		/**
		 * Called by Application when the window has been resized successfully.
		 */
		void onResized();
		/**
		 * Called by Application when the window has been updated successfully.
		 */
		void onUpdated();

		// only used by Window itself
		void init();
		void passToCtrl(const KeyEvent &e,uchar event);
		void passToCtrl(const MouseEvent &e,uchar event);
		void passMouseToCtrl(Control *c,const MouseEvent& e);
		void resize(short width,short height);
		void resizeTo(const Size &size);
		void move(short x,short y);
		void moveTo(const Pos &pos);
		void resizeMove(short x,short width,short height);
		void resizeMoveTo(gpos_t x,const Size &size);
		void prepareResize(const Pos &pos,const Size &size);

	private:
		gwinid_t _id;
		bool _created;
		Style _style;
		bool _inTitle;
		bool _inResizeLeft;
		bool _inResizeRight;
		bool _inResizeBottom;
		bool _isActive;
		Pos _movePos;
		Size _resizeSize;
		GraphicsBuffer *_gbuf;
	protected:
		std::shared_ptr<WindowTitleBar> _header;
		std::shared_ptr<Panel> _body;
		std::list<Control*> _tabCtrls;
		std::list<Control*>::iterator _tabIt;
	};

	/**
	 * Writes the window to the given stream
	 */
	std::ostream &operator<<(std::ostream &s,const Window &w);
}
