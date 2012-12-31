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
#include <gui/graphics/color.h>
#include <vector>

namespace gui {
	/**
	 * This class holds all colors that should be used for an ui-element. That is, each ui-element
	 * holds an instance of this class. Additionally, the Application class creates an instance of
	 * this class and puts in all default colors. By default, the instance for the ui-elements
	 * contains no colors at all but only a pointer to the theme-instance Application has created.
	 * This way, without intervening the default colors are used. But the user of an ui-element
	 * may overwrite all colors for a specific control to change its appearance.
	 */
	class Theme {
	public:
		/**
		 * The type for the color-constants
		 */
		typedef size_t colid_type;

	private:
		typedef unsigned int bitmap_type;

	public:
		// the different purposes for which colors are used
		static const colid_type CTRL_BACKGROUND		= 0;
		static const colid_type CTRL_FOREGROUND		= 1;
		static const colid_type CTRL_BORDER			= 2;
		static const colid_type CTRL_LIGHTBORDER	= 3;
		static const colid_type CTRL_DARKBORDER		= 4;
		static const colid_type CTRL_LIGHTBACK		= 5;
		static const colid_type CTRL_DARKBACK		= 6;
		static const colid_type BTN_BACKGROUND		= 7;
		static const colid_type BTN_FOREGROUND		= 8;
		static const colid_type SEL_BACKGROUND		= 9;
		static const colid_type SEL_FOREGROUND		= 10;
		static const colid_type TEXT_BACKGROUND		= 11;
		static const colid_type TEXT_FOREGROUND		= 12;
		static const colid_type WIN_TITLE_ACT_BG	= 13;
		static const colid_type WIN_TITLE_ACT_FG	= 14;
		static const colid_type WIN_TITLE_INACT_BG	= 15;
		static const colid_type WIN_TITLE_INACT_FG	= 16;
		static const colid_type WIN_BORDER			= 17;

	private:
		static const colid_type PADDING				= 18;
		static const colid_type TEXT_PADDING		= 19;

	public:
		/**
		 * Constructor
		 *
		 * @param def the default theme (NULL if no default should be used)
		 */
		Theme(const Theme *def)
			: _default(def), _present(0), _padding(0), _textPadding(0), _colors(NULL) {
		};
		/**
		 * Copy-constructor
		 */
		Theme(const Theme& t)
			: _default(t._default), _present(t._present), _padding(t._padding),
			  _textPadding(t._textPadding), _colors() {
			_colors = new std::vector<Color>(*t._colors);
		};
		/**
		 * Destructor
		 */
		~Theme() {
			delete _colors;
		};

		/**
		 * Assignment-operator
		 */
		Theme& operator=(const Theme& t);

		/**
		 * @return the padding
		 */
		gsize_t getPadding() const;

		/**
		 * Sets the padding to the given value, i.e. overwrites the default-one for the attached
		 * ui-element.
		 *
		 * @param pad the new value
		 */
		void setPadding(gsize_t pad);

		/**
		 * @return the padding that is used for text
		 */
		gsize_t getTextPadding() const;

		/**
		 * Sets the padding for text to the given value, i.e. overwrites the default-one for the
		 * attached ui-element.
		 *
		 * @param pad the new value
		 */
		void setTextPadding(gsize_t pad);

		/**
		 * Returns the color-instance for the given color-id. If not present in the current theme,
		 * the color in the default-theme will be returned.
		 *
		 * @param id the color-id
		 * @return the color
		 */
		const Color& getColor(colid_type id) const;

		/**
		 * Sets the color for the given id. That is, the color in this theme will be overwritten,
		 * so that the color in the default-theme will not be used anymore.
		 *
		 * @param id the color-id
		 * @param c the new color
		 */
		void setColor(colid_type id,const Color& c);

		/**
		 * Removes the color with given id from this theme, i.e. the default color is used again.
		 *
		 * @param id the color-id
		 */
		inline void unsetColor(colid_type id) {
			_present &= ~(1 << id);
		};

	private:
		const Theme *_default;
		bitmap_type _present;
		gsize_t _padding;
		gsize_t _textPadding;
		std::vector<Color> *_colors;
	};
}
