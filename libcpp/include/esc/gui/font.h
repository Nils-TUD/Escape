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
			static const u32 charWidth = 6;
			static const u32 charHeight = 8;

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

			inline u32 getWidth() {
				return charWidth;
			};
			inline u32 getHeight() {
				return charHeight;
			};
			inline u32 getStringWidth(const String &str) {
				return str.length() * charWidth;
			};
			char *getChar(char c);

		private:
			static char _font[][charHeight * charWidth];
		};
	}
}

#endif /* FONT_H_ */
