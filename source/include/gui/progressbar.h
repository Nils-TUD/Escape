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

#include <sys/common.h>
#include <gui/graphics/color.h>
#include <gui/control.h>
#include <string>

namespace gui {
	class ProgressBar : public Control {
	public:
		ProgressBar(const std::string &text)
			: Control(), _position(0), _text(text) {
		}
		ProgressBar(const std::string &text,const Pos &pos,const Size &size)
			: Control(pos,size), _position(0), _text(text) {
		}

		size_t getPosition() const {
			return _position;
		}
		void setPosition(size_t pos) {
			_position = std::min((size_t)100,pos);
			makeDirty(true);
		}
		const std::string &getText() const {
			return _text;
		}
		void setText(const std::string &text) {
			_text = text;
			makeDirty(true);
		}

		virtual void print(std::ostream &os, bool rec = true, size_t indent = 0) const {
			UIElement::print(os, rec, indent);
			os << " text='" << _text << "' pos=" << _position;
		}

	protected:
		virtual void paint(Graphics &g);

	private:
		virtual Size getPrefSize() const;

		size_t _position;
		std::string _text;
	};
}
