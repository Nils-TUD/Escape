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
#include <gui/application.h>
#include <ostream>

namespace gui {
	class Color {
	public:
		typedef uint32_t color_type;
		typedef uint8_t comp_type;

		Color(color_type color = 0) : _color(color) {
		};
		Color(comp_type red,comp_type green,comp_type blue,comp_type alpha = 0) : _color(0) {
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

		inline color_type getColor() const {
			return _color;
		};
		inline void setColor(color_type color) {
			_color = color;
		};
		inline void setColor(comp_type red,comp_type green,comp_type blue,comp_type alpha = 0) {
			_color = (alpha << 24) | (red << 16) | (green << 8) | blue;
		};

		inline comp_type getRed() const {
			return (comp_type)(_color >> 16);
		};
		inline comp_type getGreen() const {
			return (comp_type)(_color >> 8);
		};
		inline comp_type getBlue() const {
			return (comp_type)(_color & 0xFF);
		};
		inline comp_type getAlpha() const {
			return (comp_type)(_color >> 24);
		};
		color_type toCurMode() const;

	private:
		color_type _color;
	};

	std::ostream &operator<<(std::ostream &s,const Color &c);
}

#endif /* COLOR_H_ */
