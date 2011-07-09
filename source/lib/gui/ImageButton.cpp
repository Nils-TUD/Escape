/**
 * $Id: button.cpp 965 2011-07-07 10:56:45Z nasmussen $
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
	void ImageButton::paintBorder(Graphics &g) {
		if(_border)
			Button::paintBorder(g);
	}

	void ImageButton::paintBackground(Graphics &g) {
		gpos_t x = (getWidth() / 2) - (_img->getWidth() / 2);
		gpos_t y = (getHeight() / 2) - (_img->getHeight() / 2);
		_img->paint(g,x,y);
	}
}
