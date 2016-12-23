/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <gui/graphics/color.h>
#include <sys/common.h>
#include <vector>

namespace esc {
class IStream;
class OStream;
}

namespace gui {
	/**
	 * This class holds the colors and padding values, loaded from file. There is one instance per
	 * application. The Theme class refers to this class to get these values. This allows us to load
	 * a new theme from file and simply let all controls repaint themselves to apply a new theme.
	 */
	class BaseTheme {
	public:
		/**
		 * The type for the color-constants
		 */
		typedef size_t colid_type;

		/**
		 * Number of theme colors
		 */
		static const size_t COLOR_NUM	= 20;

		/**
		 * Constructor
		 *
		 * @param def the default theme (nullptr if no default should be used)
		 */
		BaseTheme()
			: _padding(0), _textPadding(0), _colors() {
			_colors.reserve(COLOR_NUM);
		}

		/**
		 * @return the padding
		 */
		gsize_t getPadding() const {
			return _padding;
		}
		/**
		 * Sets the padding.
		 *
		 * @param pad the new value
		 */
		void setPadding(gsize_t pad) {
			_padding = pad;
		}

		/**
		 * @return the padding that is used for text
		 */
		gsize_t getTextPadding() const {
			return _textPadding;
		}
		/**
		 * Sets the text padding.
		 *
		 * @param pad the new value
		 */
		void setTextPadding(gsize_t pad) {
			_textPadding = pad;
		}

		/**
		 * Returns the color-instance for the given color-id.
		 *
		 * @param id the color-id
		 * @return the color
		 */
		const Color& getColor(colid_type id) const {
			return _colors[id];
		}

		/**
		 * Sets the given color.
		 *
		 * @param id the color id
		 * @param col the color
		 */
		void setColor(colid_type id,const Color &col) {
			_colors[id] = col;
		}

		/**
		 * Writes the theme to the given output stream.
		 *
		 * @param os the output stream
		 */
		void serialize(esc::OStream &os);

		/**
		 * Reads a theme from given input stream.
		 *
		 * @param is the input stream
		 * @return the theme
		 */
		static BaseTheme unserialize(esc::IStream &is);

	private:
		void init(const BaseTheme &t);

		gsize_t _padding;
		gsize_t _textPadding;
		std::vector<Color> _colors;
	};
}
