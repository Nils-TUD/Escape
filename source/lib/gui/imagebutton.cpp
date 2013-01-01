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
#include <gui/imagebutton.h>

namespace gui {
	Size ImageButton::getPrefSize() const {
		gsize_t pad = getTheme().getPadding();
		return _img->getSize() + Size(pad * 2,pad * 2);
	}

	void ImageButton::paintBorder(Graphics &g) {
		if(_border)
			Button::paintBorder(g);
	}

	void ImageButton::paintBackground(Graphics &g) {
		gpos_t x = (getSize().width / 2) - (_img->getSize().width / 2);
		gpos_t y = (getSize().height / 2) - (_img->getSize().height / 2);
		_img->paint(g,Pos(x,y));
	}
}
