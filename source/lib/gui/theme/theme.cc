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

#include <esc/stream/istream.h>
#include <esc/stream/ostream.h>
#include <gui/theme/theme.h>
#include <gui/application.h>
#include <sys/common.h>
#include <stdio.h>

using namespace std;

namespace gui {
	Theme& Theme::operator=(const Theme& t) {
		if(&t == this)
			return *this;
		init(t);
		_colors = t._colors;
		return *this;
	}
	Theme& Theme::operator=(Theme &&t) {
		init(t);
		_colors = std::move(t._colors);
		t._colors = nullptr;
		return *this;
	}

	void Theme::init(const Theme &t) {
		_present = t._present;
		_padding = t._padding;
		_textPadding = t._textPadding;
		_dirty = t._dirty;
	}

	gsize_t Theme::getPadding() const {
		if(_present & (1 << PADDING))
			return _padding;
		return Application::getInstance()->getBaseTheme().getPadding();
	}
	void Theme::setPadding(gsize_t pad) {
		_padding = pad;
		_present |= 1 << PADDING;
		_dirty = true;
	}

	gsize_t Theme::getTextPadding() const {
		if(_present & (1 << TEXT_PADDING))
			return _textPadding;
		return Application::getInstance()->getBaseTheme().getTextPadding();
	}
	void Theme::setTextPadding(gsize_t pad) {
		_textPadding = pad;
		_present |= 1 << TEXT_PADDING;
		_dirty = true;
	}

	const Color& Theme::getColor(colid_type id) const {
		const BaseTheme &base = Application::getInstance()->getBaseTheme();
		if(_present & (1 << id))
			return base.getColor((*_colors)[id]);
		return base.getColor(id);
	}
	void Theme::setColor(colid_type id,colid_type baseid) {
		if(_colors == nullptr)
			_colors = new vector<colid_type>();
		if(id >= _colors->size())
			_colors->reserve(id + 1);
		(*_colors)[id] = baseid;
		_present |= 1 << id;
		_dirty = true;
	}
}
