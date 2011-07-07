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
#include <gui/common.h>
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

		static tWinId NEXT_TMP_ID;

		static const tSize MIN_WIDTH = 40;
		static const tSize MIN_HEIGHT = 40;

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
		Window(const string &title,tCoord x,tCoord y,tSize width,tSize height,
				uchar style = STYLE_DEFAULT);
		Window(const Window &w);
		virtual ~Window();
		Window &operator=(const Window &w);

		inline tWinId getId() const {
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
		inline tSize getTitleBarHeight() const {
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
		void resizeTo(tSize width,tSize height);
		void move(short x,short y);
		void moveTo(tCoord x,tCoord y);
		void add(Control &c);

	protected:
		void paintTitle(Graphics &g);

		inline tWinId getWindowId() const {
			return _id;
		};

	private:
		void init();
		void passToCtrl(const KeyEvent &e,uchar event);
		void passToCtrl(const MouseEvent &e,uchar event);
		void setActive(bool active);
		void onCreated(tWinId id);
		void update(tCoord x,tCoord y,tSize width,tSize height);
		void resizeMove(short x,short width,short height);
		void resizeMoveTo(tCoord x,tSize width,tSize height);
		inline tCoord getMoveX() const {
			return _moveX;
		};
		inline tCoord getMoveY() const {
			return _moveY;
		};
		inline tSize getResizeWidth() const {
			return _resizeWidth;
		};
		inline tSize getResizeHeight() const {
			return _resizeHeight;
		};

	private:
		tWinId _id;
		bool _created;
		uchar _style;
		string _title;
		tSize _titleBarHeight;
		bool _inTitle;
		bool _inResizeLeft;
		bool _inResizeRight;
		bool _inResizeBottom;
		bool _isActive;
		tCoord _moveX;
		tCoord _moveY;
		tSize _resizeWidth;
		tSize _resizeHeight;
	protected:
		int _focus;
		vector<Control*> _controls;
	};

	std::ostream &operator<<(std::ostream &s,const Window &w);
}

#endif /* WINDOW_H_ */
