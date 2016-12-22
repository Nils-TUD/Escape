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
#include <gui/theme.h>
#include <sys/common.h>
#include <stdio.h>

using namespace std;

namespace gui {
	static const char *names[] = {
		"CTRL_BACKGROUND",
		"CTRL_FOREGROUND",
		"CTRL_BORDER",
		"CTRL_LIGHTBORDER",
		"CTRL_DARKBORDER",
		"CTRL_LIGHTBACK",
		"CTRL_DARKBACK",
		"BTN_BACKGROUND",
		"BTN_FOREGROUND",
		"SEL_BACKGROUND",
		"SEL_FOREGROUND",
		"TEXT_BACKGROUND",
		"TEXT_FOREGROUND",
		"WIN_TITLE_ACT_BG",
		"WIN_TITLE_ACT_FG",
		"WIN_TITLE_INACT_BG",
		"WIN_TITLE_INACT_FG",
		"WIN_BORDER",
		"PADDING",
		"TEXT_PADDING",
	};

	static size_t findSetting(const std::string &name) {
		for(size_t i = 0; i < ARRAY_SIZE(names); ++i) {
			if(strcmp(names[i],name.c_str()) == 0)
				return i;
		}
		throw runtime_error("Unable to find setting '" + name + "'");
	}

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
	const Theme::colid_type Theme::TEXT_PADDING			= 19;

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
		return *this;
	}

	void Theme::init(const Theme &t) {
		_default = t._default;
		_present = t._present;
		_padding = t._padding;
		_textPadding = t._textPadding;
		_dirty = t._dirty;
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

	void Theme::serialize(esc::OStream &os) {
		for(size_t i = 0; i < ARRAY_SIZE(names) - 2; ++i) {
			const Color &col = getColor(i);
			os << names[i] << " = rgb(" << esc::fmt(col.getRed(),"#x") << ", "
										<< esc::fmt(col.getGreen(),"#x") << ", "
										<< esc::fmt(col.getBlue(),"#x") << ")\n";
		}

		os << "PADDING = " << getPadding() << "\n";
		os << "TEXT_PADDING = " << getTextPadding() << "\n";
	}

	Theme Theme::unserialize(esc::IStream &is) {
		Theme t(nullptr);
		size_t unlimited = std::numeric_limits<size_t>::max();
		while(is.good()) {
			std::string name;
			is >> name;
			// ignore empty lines
			if(name.empty())
				continue;

			is.ignore(unlimited,'=');

			size_t id = findSetting(name);
			switch(id) {
				case PADDING: {
					gsize_t pad;
					is >> pad;
					t.setPadding(pad);
					break;
				}

				case TEXT_PADDING: {
					gsize_t pad;
					is >> pad;
					t.setTextPadding(pad);
					break;
				}

				default: {
					is.ignore(unlimited,'(');

					Color::comp_type comps[3];
					for(size_t i = 0; i < 3; ++i) {
						is >> comps[i];
						if(i < 2 && is.get() != ',')
							throw runtime_error("Unexpected token");
					}
					t.setColor(id, Color(comps[0],comps[1],comps[2]));

					if(is.get() != ')')
						throw runtime_error("Unexpected token");
					break;
				}
			}
		}
		return t;
	}
}
