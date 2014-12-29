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
#include <img/image.h>
#include <string>

namespace img {

class PNGImage : public Image {
public:
	struct Chunk {
		uint32_t length;
		char type[4];
	} A_PACKED;

	struct IHDR {
		friend std::ostream &operator<<(std::ostream &os,const IHDR &h);

		uint32_t width;
		uint32_t height;
		uint8_t bitDepth;
		uint8_t colorType;
		uint8_t comprMethod;
		uint8_t filterMethod;
		uint8_t interlaceMethod;
	} A_PACKED;

	struct tIME {
		friend std::ostream &operator<<(std::ostream &os,const tIME &tm);

		uint16_t year;
		uint8_t month;
		uint8_t day;
		uint8_t hour;
		uint8_t minute;
		uint8_t second;
	} A_PACKED;

	enum ColorType {
		CT_GRAYSCALE		= 0,
		CT_RGB				= 2,
		CT_PALETTE			= 3,
		CT_GRAYSCALE_ALPHA	= 4,
		CT_RGB_ALPHA		= 6
	};
	enum CompressionMethod {
		CM_DEFLATE			= 0
	};
	enum FilterMethod {
		FM_ADAPTIVE			= 0
	};
	enum InterlaceMethod {
		IM_NONE				= 0,
		IM_ADAM7			= 1
	};
	enum FilterType {
		FT_NONE				= 0,
		FT_SUB				= 1,
		FT_UP				= 2,
		FT_AVERAGE			= 3,
		FT_PAETH			= 4
	};

	static const size_t SIG_LEN;
	static const uint8_t SIG[];

	explicit PNGImage(const std::shared_ptr<Painter> &painter,const std::string &filename)
		: Image(painter), _header(), _pixels(), _logbpp() {
		load(filename);
		applyFilters();
	}
	~PNGImage() {
		delete[] _pixels;
	}

	virtual void getSize(gsize_t *width,gsize_t *height) const {
		*width = _header.width;
		*height = _header.height;
	}
	virtual void paint(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

private:
	void load(const std::string &filename);
	uint8_t left(size_t off,size_t x);
	uint8_t above(size_t off,size_t y);
	uint8_t leftabove(size_t off,size_t x,size_t y);
	uint8_t paethPredictor(uint8_t a,uint8_t b,uint8_t c);
	void applyFilters();

	IHDR _header;
	uint8_t *_pixels;
	int _logbpp;
};

}
