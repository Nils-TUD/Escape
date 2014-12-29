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

#include <img/bitmapimage.h>
#include <esc/rawfile.h>

namespace img {


void BitmapImage::paintRGB(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	size_t bitCount = _infoHeader->bitCount;
	gpos_t w = _infoHeader->width, h = _infoHeader->height;
	gsize_t colBytes = bitCount / 8;
	gpos_t pad = (colBytes * w) % 4;
	pad = pad ? 4 - pad : 0;

	uint8_t *data = _data + (h - 1) * ((w * colBytes) + pad);
	if(bitCount == 8) {
		for(gpos_t cy = y + height - 1; cy >= y; cy--) {
			gpos_t xend = x + width;
			for(gpos_t cx = x; cx < xend; cx++) {
				uint32_t col = data[cx];
				if(EXPECT_TRUE(col != TRANSPARENT))
					_painter->paintPixel(cx,height - 1 - cy,_colorTable[col]);
			}
			data -= w + pad;
		}
	}
	else if(bitCount == 16) {
		for(gpos_t cy = y + height - 1; cy >= y; cy--) {
			gpos_t xend = x + width;
			for(gpos_t cx = x; cx < xend; cx++) {
				uint32_t col = *(uint16_t*)(data + (cx << 1));
				if(EXPECT_TRUE(col != TRANSPARENT))
					_painter->paintPixel(cx,height - 1 - cy,col);
			}
			data -= (w << 1) + pad;
		}
	}
	else if(bitCount == 24) {
		for(gpos_t cy = y + height - 1; cy >= y; cy--) {
			gpos_t xend = x + width;
			for(gpos_t cx = x; cx < xend; cx++) {
				/* cx * 3 = cx << 1 + cx */
				uint8_t *cdata = data + (cx << 1) + cx;
				uint32_t col = (cdata[0] | (cdata[1] << 8) | (cdata[2] << 16));
				if(EXPECT_TRUE(col != TRANSPARENT))
					_painter->paintPixel(cx,height - 1 - cy,col);
			}
			data -= (w << 1) + w + pad;
		}
	}
	else {
		for(gpos_t cy = y + height - 1; cy >= y; cy--) {
			gpos_t xend = x + width;
			for(gpos_t cx = x; cx < xend; cx++) {
				uint32_t col = *(uint32_t*)(data + (cx << 2));
				if(EXPECT_TRUE(col != TRANSPARENT))
					_painter->paintPixel(cx,height - 1 - cy,col);
			}
			data -= (w << 2) + pad;
		}
	}
}

void BitmapImage::paintBitfields(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	gpos_t w = _infoHeader->width, h = _infoHeader->height;
	gpos_t pad = w % 4;
	pad = pad ? 4 - pad : 0;

	uint32_t redmask = _infoHeader->redmask;
	uint32_t greenmask = _infoHeader->greenmask;
	uint32_t bluemask = _infoHeader->bluemask;
	uint32_t alphamask = _infoHeader->alphamask;
	int redshift = getShift(redmask);
	int greenshift = getShift(greenmask);
	int blueshift = getShift(bluemask);
	int alphashift = getShift(alphamask);

	// we know that bitdepth is 32
	assert(_infoHeader->bitCount == 32);
	uint8_t *data = _data + (h - 1) * ((w << 2) + pad);
	for(gpos_t cy = y + height - 1; cy >= y; cy--) {
		gpos_t xend = x + width;
		for(gpos_t cx = x; cx < xend; cx++) {
			uint32_t col = *(uint32_t*)(data + (cx << 2));
			uint32_t red = (col & redmask) >> redshift;
			uint32_t green = (col & greenmask) >> greenshift;
			uint32_t blue = (col & bluemask) >> blueshift;
			uint32_t alpha = (col & alphamask) >> alphashift;
			col = ((256 - alpha) << 24) | (red << 16) | (green << 8) | blue;
			if(EXPECT_TRUE(col != TRANSPARENT))
				_painter->paintPixel(cx,height - 1 - cy,col);
		}
		data -= (w << 2) + pad;
	}
}

uint BitmapImage::getShift(uint32_t val) {
	uint c = 0;
	for(; (val & 0x1) == 0; val >>= 1, ++c)
		;
	return c;
}

void BitmapImage::paint(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	if(_data == nullptr)
		return;
	switch(_infoHeader->compression) {
		case BI_RGB:
			paintRGB(x,y,width,height);
			break;

		case BI_BITFIELDS:
			paintBitfields(x,y,width,height);
			break;

		case BI_RLE4:
			// TODO
			break;
		case BI_RLE8:
			// TODO
			break;
	}
}

void BitmapImage::loadFromFile(const std::string &filename) {
	// read header
	size_t headerSize = sizeof(FileHeader) + sizeof(InfoHeader);
	uint8_t *header = new uint8_t[headerSize];
	esc::rawfile f;

	try {
		f.open(filename,esc::rawfile::READ);
		f.read(header,sizeof(uint8_t),headerSize);
	}
	catch(esc::default_error &e) {
		throw img_load_error(filename + ": Unable to open or read header: " + e.what());
	}

	_fileHeader = (FileHeader*)header;
	_infoHeader = (InfoHeader*)(_fileHeader + 1);

	// check header-type
	if(_fileHeader->type[0] != 'B' || _fileHeader->type[1] != 'M')
		throw img_load_error(filename + ": Invalid image-header ('BM' expected)");

	// determine color-table-size
	_tableSize = 0;
	if(_infoHeader->colorsUsed == 0) {
		if(_infoHeader->bitCount == 8)
			_tableSize = 1 << _infoHeader->bitCount;
	}
	else
		_tableSize = _infoHeader->colorsUsed;

	uint16_t bitCount = _infoHeader->bitCount;
	if(bitCount != 8 && bitCount != 16 && bitCount != 24 && bitCount != 32)
		throw img_load_error(filename + ": Invalid bitdepth: 8,16,24,32 are supported");

	// read color-table, if present
	if(_tableSize > 0) {
		_colorTable = new uint32_t[_tableSize];
		try {
			f.read(_colorTable,sizeof(uint32_t),_tableSize);
		}
		catch(esc::default_error &e) {
			throw img_load_error(filename + ": Unable to read color-table: " + e.what());
		}
	}

	// now read the data
	if(_infoHeader->compression == BI_RGB) {
		size_t bytesPerLine;
		bytesPerLine = _infoHeader->width * (_infoHeader->bitCount / 8);
		bytesPerLine = ROUND_UP(bytesPerLine,sizeof(uint32_t));
		_dataSize = bytesPerLine * _infoHeader->height;
	}
	else if(_infoHeader->compression == BI_BITFIELDS) {
		if(bitCount != 32)
			throw img_load_error(filename + ": Bitfields are only support for bitdepth 32");
		_dataSize = _infoHeader->sizeImage;
	}
	else
		throw img_load_error(filename + ": Unsupported compression " + _infoHeader->compression);

	_data = new uint8_t[_dataSize];
	try {
		f.seek(_fileHeader->dataOffset,esc::rawfile::SET);
		size_t bufsize = 8192;
		size_t pos = 0;
		while(pos < _dataSize) {
			size_t amount = std::min(_dataSize - pos,bufsize);
			pos += f.read(_data + pos,1,amount);
		}
	}
	catch(esc::default_error &e) {
		throw img_load_error(filename + ": Unable to read image-data: " + e.what());
	}
}

}
