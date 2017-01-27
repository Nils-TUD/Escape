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

#include <esc/vthrow.h>
#include <sys/common.h>
#include <exception>
#include <memory>

namespace img {

typedef esc::default_error img_load_error;

class Painter {
public:
	virtual ~Painter() {
	}

	virtual void paintPixel(gpos_t x,gpos_t y,uint32_t col) = 0;
};

class Image {
protected:
	explicit Image(const std::shared_ptr<Painter> &painter) : _painter(painter) {
	}

public:
	virtual ~Image() {
	}

	static Image *loadImage(const std::shared_ptr<Painter> &painter,const std::string& path);

	std::shared_ptr<Painter> getPainter() const {
		return _painter;
	}

	virtual void getSize(gsize_t *width,gsize_t *height) const = 0;
	virtual void paint(gpos_t x,gpos_t y,gsize_t width,gsize_t height) = 0;

protected:
	std::shared_ptr<Painter> _painter;
};

}
