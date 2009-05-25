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
#include <esc/stream.h>

namespace esc {
	namespace gui {
		Color Color::fromBits(u32 col,u8 bits) {
			switch(bits) {
				case 32:
					return from32Bit(col);
				case 24:
					return from24Bit(col);
				case 16:
					return from16Bit(col);
				case 8:
					return from8Bit(col);
				default:
					return Color(0);
			}
		}

		u32 Color::toBits(u8 bits) const {
			switch(bits) {
				case 32:
					return to32Bit();
				case 24:
					return to24Bit();
				case 16:
					return to16Bit();
				case 8:
					return to8Bit();
				default:
					return 0;
			}
		}

		Stream &operator<<(Stream &s,const Color &c) {
			s << "Color[" << c.getRed() << "," << c.getGreen() << "," << c.getBlue();
			s << "," << c.getAlpha() << "]";
			return s;
		}
	}
}
