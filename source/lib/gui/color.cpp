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
#include <gui/color.h>

namespace gui {
	Color::color_type Color::toCurMode() const {
		const sVESAInfo *vesaInfo = Application::getInstance()->getVesaInfo();
		comp_type red = getRed() >> (8 - vesaInfo->redMaskSize);
		comp_type green = getGreen() >> (8 - vesaInfo->greenMaskSize);
		comp_type blue = getBlue() >> (8 - vesaInfo->blueMaskSize);
		color_type val = (red << vesaInfo->redFieldPosition) |
				(green << vesaInfo->greenFieldPosition) |
				(blue << vesaInfo->blueFieldPosition);
		if(vesaInfo->bitsPerPixel == 32)
			val |= (color_type)getAlpha() << 24;
		return val;
	}

	std::ostream &operator<<(std::ostream &s,const Color &c) {
		s << "Color[" << c.getRed() << "," << c.getGreen() << "," << c.getBlue();
		s << "," << c.getAlpha() << "]";
		return s;
	}
}
