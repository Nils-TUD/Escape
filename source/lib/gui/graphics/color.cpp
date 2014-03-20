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

#include <esc/common.h>
#include <gui/graphics/color.h>
#include <gui/application.h>

using namespace std;

namespace gui {
	Color::color_type Color::toCurMode() const {
		const ipc::Screen::Mode *mode = Application::getInstance()->getScreenMode();
		comp_type red = getRed() >> (8 - mode->redMaskSize);
		comp_type green = getGreen() >> (8 - mode->greenMaskSize);
		comp_type blue = getBlue() >> (8 - mode->blueMaskSize);
		color_type val = (red << mode->redFieldPosition) |
				(green << mode->greenFieldPosition) |
				(blue << mode->blueFieldPosition);
		if(mode->bitsPerPixel == 32)
			val |= (color_type)getAlpha() << 24;
		return val;
	}

	ostream &operator<<(ostream &s,const Color &c) {
		s << "Color[" << c.getRed() << "," << c.getGreen() << "," << c.getBlue();
		s << "," << c.getAlpha() << "]";
		return s;
	}
}
