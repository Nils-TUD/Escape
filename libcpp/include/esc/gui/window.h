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
#include <esc/gui/common.h>
#include <esc/gui/graphics.h>
#include <esc/gui/uielement.h>
#include <esc/gui/application.h>
#include <esc/gui/control.h>
#include <esc/gui/color.h>
#include <esc/string.h>

namespace esc {
	namespace gui {
		class Window : public UIElement {
			friend class Application;

		private:
			static Color BGCOLOR;
			static Color TITLE_BGCOLOR;
			static Color TITLE_ACTIVE_BGCOLOR;
			static Color TITLE_FGCOLOR;
			static Color BORDER_COLOR;

			static u16 NEXT_TMP_ID;

			static const u8 MOUSE_MOVED = 0;
			static const u8 MOUSE_RELEASED = 1;
			static const u8 MOUSE_PRESSED = 2;
			static const u8 KEY_RELEASED = 3;
			static const u8 KEY_PRESSED = 4;

		public:
			static const u8 STYLE_DEFAULT = 0;
			static const u8 STYLE_POPUP = 1;

		public:
			Window(const String &title,tCoord x,tCoord y,tSize width,tSize height,
					u8 style = STYLE_DEFAULT);
			Window(const Window &w);
			virtual ~Window();
			Window &operator=(const Window &w);

			inline tWinId getId() const {
				return _id;
			};
			inline u8 getStyle() const {
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
			inline String getTitle() const {
				return _title;
			};
			inline void setTitle(const String &title) {
				_title = title;
				repaint();
			};

			virtual void onMouseMoved(const MouseEvent &e);
			virtual void onMouseReleased(const MouseEvent &e);
			virtual void onMousePressed(const MouseEvent &e);
			virtual void onKeyPressed(const KeyEvent &e);
			virtual void onKeyReleased(const KeyEvent &e);

			virtual void paint(Graphics &g);
			void move(s16 x,s16 y);
			void moveTo(tCoord x,tCoord y);
			void add(Control &c);

		protected:
			void paintTitle(Graphics &g);

			inline tWinId getWindowId() const {
				return _id;
			};

		private:
			void init();
			void passToCtrl(const KeyEvent &e,u8 event);
			void passToCtrl(const MouseEvent &e,u8 event);
			void setActive(bool active);
			void onCreated(tWinId id);

		private:
			tWinId _id;
			bool _created;
			u8 _style;
			String _title;
			tSize _titleBarHeight;
			bool _inTitle;
			bool _isActive;
		protected:
			s32 _focus;
			Vector<Control*> _controls;
		};

		Stream &operator<<(Stream &s,const Window &w);
	}
}

#endif /* WINDOW_H_ */
