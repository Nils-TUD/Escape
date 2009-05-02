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
			Button(const String &text,tCoord x,tCoord y,tSize width,tSize height)
				: Control(x,y,width,height), _pressed(false), _text(text) {

			};
			virtual ~Button() {

			};

			inline bool isPressed() const {
				return _pressed;
			};
			inline String getText() const {
				return _text;
			};
			inline void setText(const String &text) {
				_text = text;
				paint();
			};

			virtual void onMousePressed(const MouseEvent &e);
			virtual void onMouseReleased(const MouseEvent &e);
			virtual void paint();

		private:
			bool _pressed;
			String _text;
		};
	}
}

#endif /* BUTTON_H_ */
