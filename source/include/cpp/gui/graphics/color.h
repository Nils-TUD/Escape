/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#pragma once

#include <esc/common.h>
#include <ostream>

namespace gui {
	/**
	 * Represents a color with 4 components: alpha, red, green and blue. The color-representation
	 * does not depend on the current color-depth. Instead, the color will be converted
	 * correspondingly if necessary.
	 */
	class Color {
	public:
		typedef uint32_t color_type;
		typedef uint8_t comp_type;

		/**
		 * Constructor
		 *
		 * @param color the color (as 32-bit value)
		 */
		Color(color_type color = 0) : _color(color) {
		}
		/**
		 * Constructor
		 *
		 * @param red the red-component
		 * @param green the green-component
		 * @param blue the blue-component
		 * @param alpha the alpha-component (0 by default)
		 */
		Color(comp_type red,comp_type green,comp_type blue,comp_type alpha = 0) : _color(0) {
			setColor(red,green,blue,alpha);
		}
		/**
		 * Destructor
		 */
		virtual ~Color() {
		}

		/**
		 * @return the color as 32-bit value
		 */
		color_type getColor() const {
			return _color;
		}
		/**
		 * Sets the color
		 *
		 * @param color the new value (32-bit value)
		 */
		void setColor(color_type color) {
			_color = color;
		}
		/**
		 * Sets the color
		 *
		 * @param red the red-component
		 * @param green the green-component
		 * @param blue the blue-component
		 * @param alpha the alpha-component (0 by default)
		 */
		void setColor(comp_type red,comp_type green,comp_type blue,comp_type alpha = 0) {
			_color = (alpha << 24) | (red << 16) | (green << 8) | blue;
		}

		/**
		 * @return the red-component
		 */
		comp_type getRed() const {
			return (comp_type)(_color >> 16);
		}
		/**
		 * @return the green-component
		 */
		comp_type getGreen() const {
			return (comp_type)(_color >> 8);
		}
		/**
		 * @return the blue-component
		 */
		comp_type getBlue() const {
			return (comp_type)(_color & 0xFF);
		}
		/**
		 * @return the alpha-component
		 */
		comp_type getAlpha() const {
			return (comp_type)(_color >> 24);
		}
		/**
		 * Converts the color to the corresponding value used in the current color-depth to
		 * represent it.
		 *
		 * @return the color
		 */
		color_type toCurMode() const;

	private:
		color_type _color;
	};

	/**
	 * Writes the given color into the given stream
	 */
	std::ostream &operator<<(std::ostream &s,const Color &c);
}
