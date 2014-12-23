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
#include <vbe/vbe.h>
#include <img/bitmapimage.h>

#include "vesascreen.h"

class VESABMPainter : public img::Painter {
public:
	void reset(VESAScreen *scr,gpos_t x,gpos_t y) {
		_scr = scr;
		_setPixel = vbe_getPixelFunc(*_scr->mode);
		_x = x;
		_y = y;
	}

	virtual void paintPixel(gpos_t x,gpos_t y,uint32_t col);

private:
	gpos_t _x;
	gpos_t _y;
	VESAScreen *_scr;
	fSetPixel _setPixel;
};

class VESAImage {
public:
	explicit VESAImage(const std::string &filename)
		: _painter(new VESABMPainter()), _img(img::Image::loadImage(_painter,filename)) {
	}

	void getSize(gsize_t *width,gsize_t *height) {
		_img->getSize(width,height);
	}

	void paint(VESAScreen *scr,gpos_t x,gpos_t y);

private:
	std::shared_ptr<VESABMPainter> _painter;
	std::shared_ptr<img::Image> _img;
};
