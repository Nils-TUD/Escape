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

#ifndef CHECKBOX_H_
#define CHECKBOX_H_

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/gui/control.h>
#include <esc/gui/color.h>

namespace esc {
	namespace gui {
		class Checkbox : public Control {
		private:
			static Color FGCOLOR;
			static Color BGCOLOR;
			static Color LIGHT_BOX_COLOR;
			static Color DARK_BOX_COLOR;
			static Color BOX_BGCOLOR;
			static const u32 CROSS_PADDING = 2;
			static const u32 TEXT_PADDING = 4;

		public:
			Checkbox(tCoord x,tCoord y,tSize width,tSize height)
				: Control(x,y,width,height), _focused(false), _checked(false), _text(String()) {
			};
			Checkbox(const String &text,tCoord x,tCoord y,tSize width,tSize height)
				: Control(x,y,width,height), _focused(false), _checked(false), _text(text) {
			};
			Checkbox(const Checkbox &b)
				: Control(b), _focused(false), _checked(b._checked), _text(b._text) {
			};
			virtual ~Checkbox() {

			};
			Checkbox &operator=(const Checkbox &b);

			inline bool isChecked() const {
				return _checked;
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
			virtual void onKeyReleased(const KeyEvent &e);
			virtual void onMouseReleased(const MouseEvent &e);
			virtual void paint(Graphics &g);

		private:
			void setChecked(bool checked);

		private:
			bool _focused;
			bool _checked;
			String _text;
		};
	}
}

#endif /* CHECKBOX_H_ */
