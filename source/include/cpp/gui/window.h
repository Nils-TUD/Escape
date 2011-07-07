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

#ifndef WINDOW_H_
#define WINDOW_H_

#include <esc/common.h>
#include <esc/io.h>
#include <esc/messages.h>
#include <gui/graphics.h>
#include <gui/uielement.h>
#include <gui/application.h>
#include <gui/control.h>
#include <gui/color.h>
#include <string>
#include <ostream>
#include <vector>

namespace gui {
	class Window : public UIElement {
		friend class Application;

	private:
		static Color BGCOLOR;
		static Color TITLE_ACTIVE_BGCOLOR;
		static Color TITLE_INACTIVE_BGCOLOR;
		static Color TITLE_FGCOLOR;
		static Color BORDER_COLOR;

		static gwinid_t NEXT_TMP_ID;

		static const gsize_t MIN_WIDTH = 40;
		static const gsize_t MIN_HEIGHT = 40;

		static const uchar MOUSE_MOVED = 0;
		static const uchar MOUSE_RELEASED = 1;
		static const uchar MOUSE_PRESSED = 2;
		static const uchar KEY_RELEASED = 3;
		static const uchar KEY_PRESSED = 4;

	public:
		static const uchar STYLE_DEFAULT = 0;
		static const uchar STYLE_POPUP = 1;
		static const uchar STYLE_DESKTOP = 2;

	public:
		Window(const string &title,gpos_t x,gpos_t y,gsize_t width,gsize_t height,
				uchar style = STYLE_DEFAULT);
		Window(const Window &w);
		virtual ~Window();
		Window &operator=(const Window &w);

		inline gwinid_t getId() const {
			return _id;
		};
		inline uchar getStyle() const {
			return _style;
		};
		inline bool isActive() const {
			return _isActive;
		};
		inline bool isCreated() const {
			return _created;
		};
		inline gsize_t getTitleBarHeight() const {
			return _titleBarHeight;
		};
		inline string getTitle() const {
			return _title;
		};
		inline void setTitle(const string &title) {
			_title = title;
			repaint();
		};

		virtual void onMouseMoved(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onKeyPressed(const KeyEvent &e);
		virtual void onKeyReleased(const KeyEvent &e);

		virtual void paint(Graphics &g);
		void resize(short width,short height);
		void resizeTo(gsize_t width,gsize_t height);
		void move(short x,short y);
		void moveTo(gpos_t x,gpos_t y);
		void add(Control &c);

	protected:
		void paintTitle(Graphics &g);

		inline gwinid_t getWindowId() const {
			return _id;
		};

	private:
		void init();
		void passToCtrl(const KeyEvent &e,uchar event);
		void passToCtrl(const MouseEvent &e,uchar event);
		void setActive(bool active);
		void onCreated(gwinid_t id);
		void update(gpos_t x,gpos_t y,gsize_t width,gsize_t height);
		void resizeMove(short x,short width,short height);
		void resizeMoveTo(gpos_t x,gsize_t width,gsize_t height);
		inline gpos_t getMoveX() const {
			return _moveX;
		};
		inline gpos_t getMoveY() const {
			return _moveY;
		};
		inline gsize_t getResizeWidth() const {
			return _resizeWidth;
		};
		inline gsize_t getResizeHeight() const {
			return _resizeHeight;
		};

	private:
		gwinid_t _id;
		bool _created;
		uchar _style;
		string _title;
		gsize_t _titleBarHeight;
		bool _inTitle;
		bool _inResizeLeft;
		bool _inResizeRight;
		bool _inResizeBottom;
		bool _isActive;
		gpos_t _moveX;
		gpos_t _moveY;
		gsize_t _resizeWidth;
		gsize_t _resizeHeight;
	protected:
		int _focus;
		vector<Control*> _controls;
	};

	std::ostream &operator<<(std::ostream &s,const Window &w);
}

#endif /* WINDOW_H_ */
