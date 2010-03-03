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

#ifndef FONT_H_
#define FONT_H_

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/string.h>

namespace esc {
	namespace gui {
		class Font {
			static const tSize charWidth = 8;
			static const tSize charHeight = 16;

		public:
			Font() {
			};
			Font(const Font &f) {
				UNUSED(f);
			};
			~Font() {
			};

			Font &operator=(const Font &f) {
				UNUSED(f);
				return *this;
			};

			inline tSize getWidth() {
				return charWidth;
			};
			inline tSize getHeight() {
				return charHeight;
			};
			inline tSize getStringWidth(const String &str) {
				return str.length() * charWidth;
			};
			inline bool isPixelSet(char c,tCoord x,tCoord y) {
				return _font[(u8)c * charHeight + y] & (1 << (charWidth - x - 1));
			};

		private:
			static char _font[];
		};
	}
}

#endif /* FONT_H_ */
