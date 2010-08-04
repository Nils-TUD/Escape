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

#ifndef COLOR_H_
#define COLOR_H_

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/gui/application.h>
#include <ostream>

namespace esc {
	namespace gui {
		class Color {
		public:
			Color(u32 color = 0) : _color(color) {
			};
			Color(u8 red,u8 green,u8 blue,u8 alpha = 0) : _color(0) {
				setColor(red,green,blue,alpha);
			};
			Color(const Color &col) : _color(col._color) {
			};
			virtual ~Color() {
			};

			Color &operator=(const Color &col) {
				_color = col._color;
				return *this;
			};

			inline u32 getColor() const {
				return _color;
			};
			inline void setColor(u32 color) {
				_color = color;
			};
			inline void setColor(u8 red,u8 green,u8 blue,u8 alpha = 0) {
				_color = (alpha << 24) | (red << 16) | (green << 8) | blue;
			};

			inline u8 getRed() const {
				return (u8)(_color >> 16);
			};
			inline u8 getGreen() const {
				return (u8)(_color >> 8);
			};
			inline u8 getBlue() const {
				return (u8)(_color & 0xFF);
			};
			inline u8 getAlpha() const {
				return (u8)(_color >> 24);
			};
			u32 toCurMode() const;

		private:
			u32 _color;
		};

		ostream &operator<<(ostream &s,const Color &c);
	}
}

#endif /* COLOR_H_ */
