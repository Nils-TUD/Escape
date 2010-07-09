/**
 * $Id: button.h 519 2010-02-22 18:57:49Z nasmussen $
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

#ifndef BUTTON_H_
#define BUTTON_H_

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/gui/control.h>
#include <esc/gui/color.h>

namespace esc {
	namespace gui {
		class Button : public Control {
		private:
			static Color BGCOLOR;
			static Color FGCOLOR;
			static Color LIGHT_BORDER_COLOR;
			static Color DARK_BORDER_COLOR;

		public:
			Button(tCoord x,tCoord y,tSize width,tSize height)
				: Control(x,y,width,height), _focused(false), _pressed(false), _text(String()) {
			};
			Button(const String &text,tCoord x,tCoord y,tSize width,tSize height)
				: Control(x,y,width,height), _focused(false), _pressed(false), _text(text) {
			};
			Button(const Button &b)
				: Control(b), _focused(false), _pressed(b._pressed), _text(b._text) {
			};
			virtual ~Button() {

			};
			Button &operator=(const Button &b);

			inline bool isPressed() const {
				return _pressed;
			};
			inline String getText() const {
				return _text;
			};
			inline void setText(const String &text) {
				_text = text;
				repaint();
			};

			virtual void onFocusGained();
			virtual void onFocusLost();
			virtual void onKeyPressed(const KeyEvent &e);
			virtual void onKeyReleased(const KeyEvent &e);
			virtual void onMousePressed(const MouseEvent &e);
			virtual void onMouseReleased(const MouseEvent &e);
			virtual void paint(Graphics &g);

		private:
			void setPressed(bool pressed);

		private:
			bool _focused;
			bool _pressed;
			String _text;
		};
	}
}

#endif /* BUTTON_H_ */
