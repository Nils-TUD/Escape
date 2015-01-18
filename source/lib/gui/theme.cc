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

#include <gui/theme.h>
#include <sys/common.h>

using namespace std;

namespace gui {
	// the different purposes for which colors are used
	const Theme::colid_type Theme::CTRL_BACKGROUND		= 0;
	const Theme::colid_type Theme::CTRL_FOREGROUND		= 1;
	const Theme::colid_type Theme::CTRL_BORDER			= 2;
	const Theme::colid_type Theme::CTRL_LIGHTBORDER		= 3;
	const Theme::colid_type Theme::CTRL_DARKBORDER		= 4;
	const Theme::colid_type Theme::CTRL_LIGHTBACK		= 5;
	const Theme::colid_type Theme::CTRL_DARKBACK		= 6;
	const Theme::colid_type Theme::BTN_BACKGROUND		= 7;
	const Theme::colid_type Theme::BTN_FOREGROUND		= 8;
	const Theme::colid_type Theme::SEL_BACKGROUND		= 9;
	const Theme::colid_type Theme::SEL_FOREGROUND		= 10;
	const Theme::colid_type Theme::TEXT_BACKGROUND		= 11;
	const Theme::colid_type Theme::TEXT_FOREGROUND		= 12;
	const Theme::colid_type Theme::WIN_TITLE_ACT_BG		= 13;
	const Theme::colid_type Theme::WIN_TITLE_ACT_FG		= 14;
	const Theme::colid_type Theme::WIN_TITLE_INACT_BG	= 15;
	const Theme::colid_type Theme::WIN_TITLE_INACT_FG	= 16;
	const Theme::colid_type Theme::WIN_BORDER			= 17;
	const Theme::colid_type Theme::PADDING				= 18;
	const Theme::colid_type Theme::TEXT_PADDING		= 19;

	Theme& Theme::operator=(const Theme& t) {
		if(&t == this)
			return *this;
		_default = t._default;
		_present = t._present;
		_colors = t._colors;
		return *this;
	}

	gsize_t Theme::getPadding() const {
		if(_present & (1 << PADDING))
			return _padding;
		if(_default)
			return _default->getPadding();
		return 0;
	}
	void Theme::setPadding(gsize_t pad) {
		_padding = pad;
		_present |= 1 << PADDING;
		_dirty = true;
	}

	gsize_t Theme::getTextPadding() const {
		if(_present & (1 << TEXT_PADDING))
			return _textPadding;
		if(_default)
			return _default->getTextPadding();
		return 0;
	}
	void Theme::setTextPadding(gsize_t pad) {
		_textPadding = pad;
		_present |= 1 << TEXT_PADDING;
		_dirty = true;
	}

	const Color& Theme::getColor(colid_type id) const {
		if(_present & (1 << id))
			return (*_colors)[id];
		if(_default)
			return _default->getColor(id);
		throw logic_error(string("Color with id ") + id + " not set");
	}
	void Theme::setColor(colid_type id,const Color& c) {
		if(_colors == nullptr)
			_colors = new vector<Color>();
		if(id >= _colors->size())
			_colors->reserve(id + 1);
		(*_colors)[id] = c;
		_present |= 1 << id;
		_dirty = true;
	}
}
