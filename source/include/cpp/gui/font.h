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
#include <string>

namespace gui {
	class Font {
		static const gsize_t charWidth = 8;
		static const gsize_t charHeight = 16;

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

		inline gsize_t getWidth() {
			return charWidth;
		};
		inline gsize_t getHeight() {
			return charHeight;
		};
		inline gsize_t getStringWidth(const std::string& str) {
			return str.length() * charWidth;
		};
		inline bool isPixelSet(char c,gpos_t x,gpos_t y) {
			return _font[(uchar)c * charHeight + y] & (1 << (charWidth - x - 1));
		};

	private:
		static char _font[];
	};
}

#endif /* FONT_H_ */
