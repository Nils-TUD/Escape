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

#include <esc/common.h>
#include <gui/theme.h>

using namespace std;

namespace gui {
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
	}
}
