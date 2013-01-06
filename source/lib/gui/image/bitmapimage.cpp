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
#include <gui/image/bitmapimage.h>
#include <rawfile.h>

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
		uint8_t *oldData,*data = _data;
		gsize_t w = _infoHeader->width, h = _infoHeader->height;
		gsize_t pw = w, ph = h, pad;
		gpos_t cx,cy;
		uint32_t lastCol = 0;
		gsize_t colBytes = bitCount / 8;
		if(!g.validateParams(x,y,pw,ph))
			return;
		g.setColor(Color(0));
		g.updateMinMax(Pos(x,y));
		g.updateMinMax(Pos(x + pw,y + ph));
		pad = w % 4;
		pad = pad ? 4 - pad : 0;
		data += (h - 1) * ((w * colBytes) + pad);

		if(bitCount <= 8) {
			for(cy = ph - 1; cy >= 0; cy--) {
				oldData = data;
				for(cx = 0; cx < w; cx++) {
					if(cx < pw) {
						uint32_t col = *data;
						paintPixel8(g,cx + x,y + (ph - 1 - cy),col,lastCol);
					}
					data += colBytes;
				}
				data = oldData - ((w * colBytes) + pad);
			}
		}
		else if(bitCount == 16) {
			for(cy = ph - 1; cy >= 0; cy--) {
				oldData = data;
				for(cx = 0; cx < w; cx++) {
					if(cx < pw) {
						uint32_t col = *(uint16_t*)data;
						paintPixel(g,cx + x,y + (ph - 1 - cy),col,lastCol);
					}
					data += colBytes;
				}
				data = oldData - ((w * colBytes) + pad);
			}
		}
		else if(bitCount == 24) {
			for(cy = ph - 1; cy >= 0; cy--) {
				oldData = data;
				for(cx = 0; cx < w; cx++) {
					if(cx < pw) {
						uint32_t col = (data[0] | (data[1] << 8) | (data[2] << 16));
						paintPixel(g,cx + x,y + (ph - 1 - cy),col,lastCol);
					}
					data += colBytes;
				}
				data = oldData - ((w * colBytes) + pad);
			}
		}
		else {
			for(cy = ph - 1; cy >= 0; cy--) {
				oldData = data;
				for(cx = 0; cx < w; cx++) {
					if(cx < pw) {
						uint32_t col = *(uint32_t*)data;
						paintPixel(g,cx + x,y + (ph - 1 - cy),col,lastCol);
					}
					data += colBytes;
				}
				data = oldData - ((w * colBytes) + pad);
			}
		}
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
		if(_data == nullptr)
			return;
		switch(_infoHeader->compression) {
			case BI_RGB:
				paintRGB(g,pos.x,pos.y);
				break;

			case BI_BITFIELDS:
				// TODO
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
		catch(io_exception &e) {
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
			if(_infoHeader->bitCount == 1 || _infoHeader->bitCount == 4 ||
					_infoHeader->bitCount == 8)
				_tableSize = 1 << _infoHeader->bitCount;
		}
		else
			_tableSize = _infoHeader->colorsUsed;

		uint16_t bitCount = _infoHeader->bitCount;
		if(bitCount != 1 && bitCount != 4 && bitCount != 8 && bitCount != 24)
			throw img_load_error(filename + "Invalid bitdepth: 1,4,8,24 are supported");

		// read color-table, if present
		if(_tableSize > 0) {
			_colorTable = new uint32_t[_tableSize];
			try {
				f.read(_colorTable,sizeof(uint32_t),_tableSize);
			}
			catch(io_exception &e) {
				throw img_load_error(filename + ": Unable to read color-table: " + e.what());
			}
		}

		// now read the data
		if(_infoHeader->compression == BI_RGB) {
			size_t bytesPerLine;
			bytesPerLine = _infoHeader->width * (_infoHeader->bitCount / 8);
			bytesPerLine = (bytesPerLine + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1);
			_dataSize = bytesPerLine * _infoHeader->height;
		}
		else {
			throw img_load_error(filename + ": Sorry, only RGB is supported");
			_dataSize = _infoHeader->sizeImage;
		}

		_data = new uint8_t[_dataSize];
		try {
			f.seek(_fileHeader->dataOffset,rawfile::SET);
			f.read(_data,sizeof(uint8_t),_dataSize);
		}
		catch(io_exception &e) {
			throw img_load_error(filename + ": Unable to read image-data: " + e.what());
		}
	}
}
