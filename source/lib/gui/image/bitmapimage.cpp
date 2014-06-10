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

#include <esc/common.h>
#include <gui/image/bitmapimage.h>
#include <rawfile.h>

using namespace std;

namespace gui {
	void BitmapImage::clone(const BitmapImage &img) {
		size_t headerSize = sizeof(sBMFileHeader) + sizeof(sBMInfoHeader);
		uint8_t *header = new uint8_t[headerSize];
		_fileHeader = (sBMFileHeader*)header;
		_infoHeader = (sBMInfoHeader*)(_fileHeader + 1);
		_tableSize = img._tableSize;
		_dataSize = img._dataSize;
		memcpy(_fileHeader,img._fileHeader,headerSize);
		_colorTable = new uint32_t[img._tableSize];
		memcpy(_colorTable,img._colorTable,img._tableSize * sizeof(uint32_t));
		_data = new uint8_t[img._dataSize];
		memcpy(_data,img._data,img._dataSize);
	}

	void BitmapImage::paintRGB(Graphics &g,gpos_t x,gpos_t y) {
		size_t bitCount = _infoHeader->bitCount;
		gpos_t w = _infoHeader->width, h = _infoHeader->height;
		gsize_t colBytes = bitCount / 8;
		gpos_t pad = w % 4;
		pad = pad ? 4 - pad : 0;

		Pos rpos(x,y);
		Size rsize(w,h);
		if(!g.validateParams(rpos,rsize))
			return;
		rpos -= Size(x,y);

		uint32_t lastCol = 0;
		g.setColor(Color(lastCol));

		uint8_t *data = _data + (h - 1) * ((w * colBytes) + pad);
		if(bitCount == 8) {
			for(gpos_t cy = rpos.y + rsize.height - 1; cy >= rpos.y; cy--) {
				gpos_t xend = rpos.x + rsize.width;
				for(gpos_t cx = rpos.x; cx < xend; cx++) {
					uint32_t col = data[cx];
					paintPixel8(g,cx + x,y + (rsize.height - 1 - cy),col,lastCol);
				}
				data -= w + pad;
			}
		}
		else if(bitCount == 16) {
			for(gpos_t cy = rpos.y + rsize.height - 1; cy >= rpos.y; cy--) {
				gpos_t xend = rpos.x + rsize.width;
				for(gpos_t cx = rpos.x; cx < xend; cx++) {
					uint32_t col = *(uint16_t*)(data + (cx << 1));
					paintPixel(g,cx + x,y + (rsize.height - 1 - cy),col,lastCol);
				}
				data -= (w << 1) + pad;
			}
		}
		else if(bitCount == 24) {
			for(gpos_t cy = rpos.y + rsize.height - 1; cy >= rpos.y; cy--) {
				gpos_t xend = rpos.x + rsize.width;
				for(gpos_t cx = rpos.x; cx < xend; cx++) {
					/* cx * 3 = cx << 1 + cx */
					uint8_t *cdata = data + (cx << 1) + cx;
					uint32_t col = (cdata[0] | (cdata[1] << 8) | (cdata[2] << 16));
					paintPixel(g,cx + x,y + (rsize.height - 1 - cy),col,lastCol);
				}
				data -= (w << 1) + w + pad;
			}
		}
		else {
			for(gpos_t cy = rpos.y + rsize.height - 1; cy >= rpos.y; cy--) {
				gpos_t xend = rpos.x + rsize.width;
				for(gpos_t cx = rpos.x; cx < xend; cx++) {
					uint32_t col = *(uint16_t*)(data + (cx << 2));
					paintPixel(g,cx + x,y + (rsize.height - 1 - cy),col,lastCol);
				}
				data -= (w << 2) + pad;
			}
		}
	}

	void BitmapImage::paintBitfields(Graphics &g,gpos_t x,gpos_t y) {
		gpos_t w = _infoHeader->width, h = _infoHeader->height;
		gpos_t pad = w % 4;
		pad = pad ? 4 - pad : 0;

		Pos rpos(x,y);
		Size rsize(w,h);
		if(!g.validateParams(rpos,rsize))
			return;
		rpos -= Size(x,y);

		uint32_t redmask = _infoHeader->redmask;
		uint32_t greenmask = _infoHeader->greenmask;
		uint32_t bluemask = _infoHeader->bluemask;
		uint32_t alphamask = _infoHeader->alphamask;
		int redshift = getShift(redmask);
		int greenshift = getShift(greenmask);
		int blueshift = getShift(bluemask);
		int alphashift = getShift(alphamask);

		uint32_t lastCol = 0;
		g.setColor(Color(lastCol));

		// we know that bitdepth is 32
		assert(_infoHeader->bitCount == 32);
		uint8_t *data = _data + (h - 1) * ((w << 2) + pad);
		for(gpos_t cy = rpos.y + rsize.height - 1; cy >= rpos.y; cy--) {
			gpos_t xend = rpos.x + rsize.width;
			for(gpos_t cx = rpos.x; cx < xend; cx++) {
				uint32_t col = *(uint32_t*)(data + (cx << 2));
				uint32_t red = (col & redmask) >> redshift;
				uint32_t green = (col & greenmask) >> greenshift;
				uint32_t blue = (col & bluemask) >> blueshift;
				uint32_t alpha = (col & alphamask) >> alphashift;
				col = ((256 - alpha) << 24) | (red << 16) | (green << 8) | blue;
				if(EXPECT_TRUE(col != TRANSPARENT)) {
					if(EXPECT_FALSE(col != lastCol)) {
						g.setColor(Color(col));
						lastCol = col;
					}
					g.doSetPixel(cx + x,y + (rsize.height - 1 - cy));
				}
			}
			data -= ((w << 2) + pad);
		}
		g.updateMinMax(Pos(rpos.x,rpos.y));
		g.updateMinMax(Pos(rpos.x + rsize.width - 1,rpos.y + rsize.height - 1));
	}

	uint BitmapImage::getShift(uint32_t val) {
		uint c = 0;
		for(; (val & 0x1) == 0; val >>= 1, ++c)
			;
		return c;
	}

	void BitmapImage::paintPixel(Graphics &g,gpos_t x,gpos_t y,uint32_t col,uint32_t &lastCol) {
		if(col != TRANSPARENT) {
			if(col != lastCol) {
				g.setColor(Color(col));
				lastCol = col;
			}
			g.doSetPixel(x,y);
		}
	}

	void BitmapImage::paintPixel8(Graphics &g,gpos_t x,gpos_t y,uint32_t col,uint32_t &lastCol) {
		if(col != TRANSPARENT) {
			if(col != lastCol) {
				g.setColor(Color(_colorTable[col]));
				lastCol = col;
			}
			g.doSetPixel(x,y);
		}
	}

	void BitmapImage::paint(Graphics &g,const Pos &pos) {
		if(_data == nullptr || g.getPixels() == NULL)
			return;
		switch(_infoHeader->compression) {
			case BI_RGB:
				paintRGB(g,pos.x,pos.y);
				break;

			case BI_BITFIELDS:
				paintBitfields(g,pos.x,pos.y);
				break;

			case BI_RLE4:
				// TODO
				break;
			case BI_RLE8:
				// TODO
				break;
		}
	}

	void BitmapImage::loadFromFile(const string &filename) {
		// read header
		size_t headerSize = sizeof(sBMFileHeader) + sizeof(sBMInfoHeader);
		uint8_t *header = new uint8_t[headerSize];
		rawfile f;

		try {
			f.open(filename,rawfile::READ);
			f.read(header,sizeof(uint8_t),headerSize);
		}
		catch(default_error &e) {
			throw img_load_error(filename + ": Unable to open or read header: " + e.what());
		}

		_fileHeader = (sBMFileHeader*)header;
		_infoHeader = (sBMInfoHeader*)(_fileHeader + 1);

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
			catch(default_error &e) {
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

		_data = new uint8_t[_dataSize];
		try {
			f.seek(_fileHeader->dataOffset,rawfile::SET);
			size_t bufsize = 8192;
			size_t pos = 0;
			while(pos < _dataSize) {
				size_t amount = std::min(_dataSize - pos,bufsize);
				f.read(_data + pos,1,amount);
				pos += amount;
			}
		}
		catch(default_error &e) {
			throw img_load_error(filename + ": Unable to read image-data: " + e.what());
		}
	}
}
