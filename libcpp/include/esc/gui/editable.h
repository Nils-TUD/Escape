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

#ifndef EDITABLE_H_
#define EDITABLE_H_

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/gui/control.h>

namespace esc {
	namespace gui {
		class Editable : public Control {
		public:
			static const u32 PADDING = 3;
			static const u32 CURSOR_WIDTH = 2;
			static const u32 CURSOR_OVERLAP = 2;
			static const Color BGCOLOR;
			static const Color FGCOLOR;
			static const Color BORDER_COLOR;
			static const Color CURSOR_COLOR;

		public:
			Editable(tCoord x,tCoord y,tSize width,tSize height)
				: Control(x,y,width,height), _cursor(0), _focused(false), _str(String()) {
			};
			Editable(const Editable &e)
				: Control(e), _cursor(e._cursor), _focused(false), _str(e._str) {
			};
			virtual ~Editable() {
			};
			Editable &operator=(const Editable &e);

			inline String getText() const {
				return _str;
			};
			inline void setText(const String &text) {
				_str = text;
				_cursor = text.length();
				repaint();
			};

			virtual void paint(Graphics &g);
			virtual void onFocusGained();
			virtual void onFocusLost();
			virtual void onKeyPressed(const KeyEvent &e);

		private:
			inline u32 getMaxCharNum(Graphics &g) {
				return (getWidth() - PADDING) / g.getFont().getWidth();
			};

		private:
			u32 _cursor;
			bool _focused;
			String _str;
		};
	}
}

#endif /* EDITABLE_H_ */
