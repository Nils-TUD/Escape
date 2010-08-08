/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/gui/common.h>
#include <esc/gui/color.h>

namespace esc {
	namespace gui {
		u32 Color::toCurMode() const {
			const sVESAInfo *vesaInfo = Application::getInstance()->getVesaInfo();
			u8 red = getRed() >> (8 - vesaInfo->redMaskSize);
			u8 green = getGreen() >> (8 - vesaInfo->greenMaskSize);
			u8 blue = getBlue() >> (8 - vesaInfo->blueMaskSize);
			u32 val = (red << vesaInfo->redFieldPosition) |
					(green << vesaInfo->greenFieldPosition) |
					(blue << vesaInfo->blueFieldPosition);
			if(vesaInfo->bitsPerPixel == 32)
				val |= (u32)getAlpha() << 24;
			return val;
		}

		std::ostream &operator<<(std::ostream &s,const Color &c) {
			s << "Color[" << c.getRed() << "," << c.getGreen() << "," << c.getBlue();
			s << "," << c.getAlpha() << "]";
			return s;
		}
	}
}
