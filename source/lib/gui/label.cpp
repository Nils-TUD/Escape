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
#include <gui/label.h>
#include <gui/control.h>

namespace gui {
	Size Label::getPrefSize() const {
		const Font &f = getGraphics()->getFont();
		gsize_t pad = getTheme().getTextPadding();
		return Size(f.getStringWidth(_text) + pad * 2,f.getSize().height + pad * 2);
	}

	void Label::paint(Graphics &g) {
		Size fontsize = g.getFont().getSize();
		gsize_t pad = getTheme().getTextPadding();
		g.setColor(getTheme().getColor(Theme::CTRL_FOREGROUND));

		gpos_t y = (getSize().height - fontsize.height) / 2;
		size_t fontlen = fontsize.width * _text.length();
		switch(_align) {
			case FRONT:
				g.drawString(pad,y,_text);
				break;
			case CENTER:
				g.drawString((getSize().width - fontlen) / 2,y,_text);
				break;
			case BACK:
				g.drawString(getSize().width - fontlen - pad,y,_text);
				break;
		}
	}
}
