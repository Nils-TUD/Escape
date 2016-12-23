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
#include <gui/theme/basetheme.h>
#include <gui/theme/theme.h>
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
		"DESKTOP_BG",
		"ERROR_COLOR",
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

	void BaseTheme::serialize(esc::OStream &os) {
		for(size_t i = 0; i < ARRAY_SIZE(names) - 2; ++i) {
			const Color &col = getColor(i);
			os << names[i] << " = rgb(" << esc::fmt(col.getRed(),"#x") << ", "
										<< esc::fmt(col.getGreen(),"#x") << ", "
										<< esc::fmt(col.getBlue(),"#x") << ")\n";
		}

		os << "PADDING = " << getPadding() << "\n";
		os << "TEXT_PADDING = " << getTextPadding() << "\n";
	}

	BaseTheme BaseTheme::unserialize(esc::IStream &is) {
		BaseTheme t;
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
				case Theme::PADDING:
					is >> t._padding;
					break;

				case Theme::TEXT_PADDING:
					is >> t._textPadding;
					break;

				default: {
					is.ignore(unlimited,'(');

					Color::comp_type comps[3];
					for(size_t i = 0; i < 3; ++i) {
						is >> comps[i];
						if(i < 2 && is.get() != ',')
							throw runtime_error("Unexpected token");
					}
					t.setColor(id,Color(comps[0],comps[1],comps[2]));

					if(is.get() != ')')
						throw runtime_error("Unexpected token");
					break;
				}
			}
		}
		return t;
	}
}
