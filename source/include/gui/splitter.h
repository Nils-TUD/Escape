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
#include <gui/enums.h>
#include <gui/control.h>

namespace gui {
	class Splitter : public Control {
		static const gsize_t LENGTH		= 8;
		static const gsize_t DEPTH		= 4;

	public:
		explicit Splitter(Orientation orientation) : Control(), _orientation(orientation) {
		}
		explicit Splitter(Orientation orientation,const Pos &pos,const Size &size)
			: Control(pos,size), _orientation(orientation) {
		}

	protected:
		virtual void paint(Graphics &g) {
			Size size = getSize();
			Size lendepth = _orientation == VERTICAL ? Size(DEPTH,LENGTH) : Size(LENGTH,DEPTH);

			g.setColor(getTheme().getColor(Theme::CTRL_BORDER));
			if(_orientation == VERTICAL)
				g.fillRect(Pos(size.width / 2 - DEPTH / 2 + 1,0),Size(DEPTH - 2,size.height));
			else
				g.fillRect(Pos(0,size.height / 2 - DEPTH / 2 + 1),Size(size.width,DEPTH - 2));

			g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
			g.fillRect(Pos(size.width / 2 - lendepth.width / 2,
				size.height / 2 - lendepth.height / 2),lendepth);
		}

	private:
		virtual Size getPrefSize() const {
			if(_orientation == VERTICAL)
				return Size(DEPTH + 4,LENGTH);
			return Size(LENGTH,DEPTH + 4);
		}

		Orientation _orientation;
	};
}
