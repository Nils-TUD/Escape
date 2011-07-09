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

#ifndef WINDOW_H_
#define WINDOW_H_

#include <esc/common.h>
#include <esc/io.h>
#include <esc/messages.h>
#include <gui/graphics.h>
#include <gui/graphicsbuffer.h>
#include <gui/uielement.h>
#include <gui/application.h>
#include <gui/button.h>
#include <gui/image.h>
#include <gui/control.h>
#include <gui/panel.h>
#include <gui/color.h>
#include <string>
#include <ostream>
#include <list>

namespace gui {
	class WindowTitleBar : public Panel, public ActionListener {
		friend class Window;

	private:
		static const char *CLOSE_IMG;

		static Color TITLE_ACTIVE_BGCOLOR;
		static Color TITLE_INACTIVE_BGCOLOR;
		static Color TITLE_FGCOLOR;

	public:
		WindowTitleBar(const string& title,gpos_t x,gpos_t y,gsize_t width,gsize_t height);
		virtual ~WindowTitleBar();
		WindowTitleBar(const WindowTitleBar& wtb);
		WindowTitleBar& operator=(const WindowTitleBar& wtb);

		/**
		 * @return the title (a copy)
		 */
		inline string getTitle() const {
			return _title;
		};
		/**
		 * Sets the title and requests a repaint
		 *
		 * @param title the new title
		 */
		inline void setTitle(const string &title) {
			_title = title;
			repaint();
		};

		virtual void actionPerformed(UIElement& el);

		virtual void resizeTo(gsize_t width,gsize_t height);
		virtual void paint(Graphics& g);

	private:
		void init();

	private:
		string _title;
		Image *_imgs[1];
		Button *_btns[1];
	};

	/**
	 * Represents a window in the GUI.
	 */
	class Window : public UIElement {
		friend class Application;
		friend class Panel;

	private:
		// colors
		static Color BGCOLOR;
		static Color BORDER_COLOR;

		// used for creation
		static gwinid_t NEXT_TMP_ID;

		// minimum window-size
		static const gsize_t MIN_WIDTH = 40;
		static const gsize_t MIN_HEIGHT = 40;

		static const gsize_t HEADER_SIZE = 20;

	public:
		/**
		 * For all ordinary windows
		 */
		static const uchar STYLE_DEFAULT = 0;
		/**
		 * This is used for popups (e.g. combobox). They are not moveable and resizeable
		 */
		static const uchar STYLE_POPUP = 1;
		/**
		 * Only for the desktop-application. Not moveable or resizeable and can't be active.
		 */
		static const uchar STYLE_DESKTOP = 2;

	public:
		/**
		 * Constructor
		 *
		 * @param title the window-title
		 * @param x the x-position
		 * @param y the y-position
		 * @param width the width
		 * @param height the height
		 * @param style the window-style (STYLE_*)
		 */
		Window(const string &title,gpos_t x,gpos_t y,gsize_t width,gsize_t height,
				uchar style = STYLE_DEFAULT);
		/**
		 * Clones the given window
		 *
		 * @param w the window
		 */
		Window(const Window &w);
		/**
		 * Destructor
		 */
		virtual ~Window();
		/**
		 * Assignment-operator
		 *
		 * @param w the window
		 */
		Window &operator=(const Window &w);

		/**
		 * @return the id of the window
		 */
		inline gwinid_t getId() const {
			return _id;
		};
		/**
		 * @return the style of the window (STYLE_*)
		 */
		inline uchar getStyle() const {
			return _style;
		};
		/**
		 * @return true if this is the active window
		 */
		inline bool isActive() const {
			return _isActive;
		};
		/**
		 * @return true if the creation of the window is finished
		 */
		inline bool isCreated() const {
			return _created;
		};
		/**
		 * @return the title (a copy)
		 */
		inline string getTitle() const {
			return _header.getTitle();
		};
		/**
		 * Sets the title and requests a repaint
		 *
		 * @param title the new title
		 */
		inline void setTitle(const string &title) {
			_header.setTitle(title);
		};
		/**
		 * @return the focused control (NULL if none)
		 */
		inline Control *getFocus() {
			return _body.getFocus();
		};
		/**
		 * Sets the focus on the given control.
		 *
		 * @param c the control
		 */
		void setFocus(Control *c);
		/**
		 * @return the height of the title-bar
		 */
		inline gsize_t getTitleBarHeight() const {
			return _header.getHeight();
		};

		/**
		 * @return the root-panel to which you can add controls
		 */
		inline Panel &getRootPanel() {
			return _body;
		};

		/**
		 * The event-callbacks
		 *
		 * @param e the event
		 */
		virtual void onMouseMoved(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onKeyPressed(const KeyEvent &e);
		virtual void onKeyReleased(const KeyEvent &e);

		/**
		 * Adds the given control as tab-control to this window
		 *
		 * @param c the control
		 */
		void appendTabCtrl(Control &c);

		/**
		 * Paints the whole window
		 *
		 * @param g the graphics-object
		 */
		virtual void paint(Graphics &g);
		/**
		 * Resizes the window by width and height.
		 *
		 * @param width the width to add to the current one
		 * @param height the height to add to the current one
		 */
		void resize(short width,short height);
		/**
		 * Resizes the window to width and height
		 *
		 * @param width the new width
		 * @param height the new height
		 */
		virtual void resizeTo(gsize_t width,gsize_t height);
		/**
		 * Moves the window by x any y
		 *
		 * @param x the amount to add to the current x-position
		 * @param y the amount to add to the current y-position
		 */
		void move(short x,short y);
		/**
		 * Moves the window to x,y.
		 *
		 * @param x the new x-position
		 * @param y the new y-position
		 */
		virtual void moveTo(gpos_t x,gpos_t y);

	protected:
		/**
		 * Paints only the title of the window
		 *
		 * @param g the graphics-object
		 */
		void paintTitle(Graphics &g);

		/**
		 * @return the window the ui-element belongs to (this)
		 */
		virtual Window *getWindow() {
			return this;
		};
		virtual const Window *getWindow() const {
			return this;
		};

	private:
		/**
		 * @return the current x-position to move to
		 */
		inline gpos_t getMoveX() const {
			return _moveX;
		};
		/**
		 * @return the current y-position to move to
		 */
		inline gpos_t getMoveY() const {
			return _moveY;
		};
		/**
		 * @return the current width to resize to
		 */
		inline gsize_t getResizeWidth() const {
			return _resizeWidth;
		};
		/**
		 * @return the current height to resize to
		 */
		inline gsize_t getResizeHeight() const {
			return _resizeHeight;
		};
		/**
		 * Called by Application when the window has been created successfully
		 *
		 * @param id the received window-id
		 */
		void onCreated(gwinid_t id);
		/**
		 * Sets whether this window is active (used by Application).
		 * Requests a repaint, if necessary.
		 *
		 * @param active the new value
		 */
		void setActive(bool active);
		/**
		 * Updates the given rectangle (copies from buffer to shared-mem and notifies vesa)
		 *
		 * @param x the x-position
		 * @param y the y-position
		 * @param width the width
		 * @param height the height
		 */
		void update(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

		// only used by Window itself
		void init();
		void passToCtrl(const KeyEvent &e,uchar event);
		void passToCtrl(const MouseEvent &e,uchar event);
		void resizeMove(short x,short width,short height);
		void resizeMoveTo(gpos_t x,gsize_t width,gsize_t height);

	private:
		gwinid_t _id;
		bool _created;
		uchar _style;
		bool _inTitle;
		bool _inResizeLeft;
		bool _inResizeRight;
		bool _inResizeBottom;
		bool _isActive;
		gpos_t _moveX;
		gpos_t _moveY;
		gsize_t _resizeWidth;
		gsize_t _resizeHeight;
		GraphicsBuffer *_gbuf;
	protected:
		WindowTitleBar _header;
		Panel _body;
		list<Control*> _tabCtrls;
		list<Control*>::iterator _tabIt;
	};

	/**
	 * Writes the window to the given stream
	 */
	std::ostream &operator<<(std::ostream &s,const Window &w);
}

#endif /* WINDOW_H_ */
