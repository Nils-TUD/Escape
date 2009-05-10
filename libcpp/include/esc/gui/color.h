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
#include <esc/stream.h>

namespace esc {
	namespace gui {
		// TODO not finished yet!
		class Color {
		public:
			static inline Color from8Bit(u8 col) {
				return Color(col);
			};
			static inline Color from16Bit(u16 col) {
				return Color((col & 0xF800) >> 11,(col & 0x7E0) >> 5,col & 0x1F);
			};
			static inline Color from24Bit(u32 col) {
				return Color(col);
			};
			static inline Color from32Bit(u32 col) {
				return Color(col);
			};
			static Color fromBits(u32 col,u8 bits) {
				switch(bits) {
					case 32:
						return from32Bit(col);
					case 24:
						return from24Bit(col);
					case 16:
						return from16Bit(col);
					case 8:
						return from8Bit(col);
					default:
						return Color(0);
				}
			};

		public:
			Color(u32 color = 0)
				: _color(color) {
			};
			Color(u8 red,u8 green,u8 blue,u8 alpha = 0) {
				setColor(red,green,blue,alpha);
			};
			Color(const Color &col)
				: _color(col._color) {
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

			inline u8 to8Bit() const {
				return _color & 0xFF;
			};
			inline u16 to16Bit() const {
				return ((getRed() & 0x1F) << 11) |
					((getGreen() & 0x3F) << 5) |
					(getBlue() & 0x1F);
			};
			inline u32 to24Bit() const {
				return _color & 0xFFFFFF;
			};
			inline u32 to32Bit() const {
				return _color;
			};
			u32 toBits(u8 bits) const {
				switch(bits) {
					case 32:
						return to32Bit();
					case 24:
						return to24Bit();
					case 16:
						return to16Bit();
					case 8:
						return to8Bit();
					default:
						return 0;
				}
			}

		private:
			u32 _color;
		};

		Stream &operator<<(Stream &s,const Color &c);
	}
}

#endif /* COLOR_H_ */
