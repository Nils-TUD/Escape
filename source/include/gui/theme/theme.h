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
#include <gui/theme/basetheme.h>
#include <sys/common.h>
#include <vector>

namespace gui {
	/**
	 * This class holds the theme settings for one UI element. The actual colors are stored in the
	 * base theme, but it is possible to change the mapping of colors.
	 */
	class Theme {
		friend class BaseTheme;

		typedef unsigned int bitmap_type;

	public:
		typedef BaseTheme::colid_type colid_type;

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
		static const colid_type DESKTOP_BG			= 18;
		static const colid_type ERROR_COLOR			= 19;

	private:
		static const colid_type PADDING				= 20;
		static const colid_type TEXT_PADDING		= 21;

	public:
		/**
		 * Constructor
		 *
		 * @param def the default theme (nullptr if no default should be used)
		 */
		Theme()
			: _present(0), _padding(0), _textPadding(0), _colors(nullptr), _dirty(false) {
			static_assert(PADDING == BaseTheme::COLOR_NUM,"COLOR_NUM is wrong");
		}
		/**
		 * Copy-constructor
		 */
		Theme(const Theme& t)
			: _present(t._present), _padding(t._padding), _textPadding(t._textPadding),
			  _colors(), _dirty(t._dirty) {
			if(t._colors)
				_colors = new std::vector<colid_type>(*t._colors);
		}
		/**
		 * Move constructor
		 */
		Theme(Theme &&t)
			: _present(t._present), _padding(t._padding), _textPadding(t._textPadding),
			  _colors(std::move(t._colors)), _dirty(t._dirty) {
	  		t._colors = nullptr;
		}
		/**
		 * Destructor
		 */
		~Theme() {
			delete _colors;
		}

		/**
		 * Assignment-operator
		 */
		Theme& operator=(const Theme &t);
		/**
		 * Move assignment-operator
		 */
		Theme& operator=(Theme &&t);

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
		 * @param id the color-id
		 * @return true if the color is set in this theme (not default)
		 */
		bool hasColor(colid_type id) const {
			return _present & (1 << id);
		}

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
		void setColor(colid_type id,colid_type baseid);

		/**
		 * Removes the color with given id from this theme, i.e. the default color is used again.
		 *
		 * @param id the color-id
		 */
		void unsetColor(colid_type id) {
			_present &= ~(1 << id);
			_dirty = true;
		}

		/**
		 * @return true if the theme has changed
		 */
		bool isDirty() const {
			return _dirty;
		}
		/**
		 * Sets whether the theme is dirty.
		 *
		 * @param dirty the new value
		 */
		void setDirty(bool dirty) {
			_dirty = dirty;
		}

	private:
		void init(const Theme &t);

		bitmap_type _present;
		gsize_t _padding;
		gsize_t _textPadding;
		std::vector<colid_type> *_colors;
		bool _dirty;
	};
}
