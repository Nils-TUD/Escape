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

		public:
			Editable(tCoord x,tCoord y,tSize width,tSize height)
				: Control(x,y,width,height), _cursor(0), _focused(false), _str(String()) {
			};
			virtual ~Editable() {
			};

			inline String getText() const {
				return _str;
			};
			inline void setText(const String &text) {
				_str = text;
				_cursor = text.length();
				paint();
			};

			virtual void paint();
			virtual void onFocusGained();
			virtual void onFocusLost();
			virtual void onKeyPressed(const KeyEvent &e);
			virtual void onKeyReleased(const KeyEvent &e);

		private:
			u32 _cursor;
			bool _focused;
			String _str;
		};
	}
}

#endif /* EDITABLE_H_ */
