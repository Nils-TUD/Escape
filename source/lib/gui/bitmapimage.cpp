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
#include <esc/debug.h>
#include <gui/bitmapimage.h>
#include <rawfile.h>

namespace gui {
	void BitmapImage::paint(Graphics &g,gpos_t x,gpos_t y) {
		if(_data == NULL)
			return;
		switch(_infoHeader->compression) {
			case BI_RGB: {
				size_t bitCount = _infoHeader->bitCount;
				uint8_t *oldData,*data = _data;
				gsize_t w = _infoHeader->width, h = _infoHeader->height;
				gsize_t pw = w, ph = h, pad;
				gpos_t cx,cy;
				size_t lastCol = 0;
				gsize_t colBytes = bitCount / 8;
				g.validateParams(x,y,pw,ph);
				g.setColor(Color(0));
				g.updateMinMax(x,y);
				g.updateMinMax(x + pw,y + ph);
				pad = w % 4;
				pad = pad ? 4 - pad : 0;
				data += (h - 1) * ((w * colBytes) + pad);
				for(cy = ph - 1; cy >= 0; cy--) {
					oldData = data;
					for(cx = 0; cx < w; cx++) {
						if(cx < pw) {
							// TODO performance might be improvable
							size_t col;
							if(bitCount <= 8)
								col = *data;
							else if(bitCount == 16)
								col = *(uint16_t*)data;
							else if(bitCount == 24)
								col = (data[0] | (data[1] << 8) | (data[2] << 16));
							else
								col = *(uint32_t*)data;
							if(col != lastCol) {
								g.setColor(Color(bitCount <= 8 ? _colorTable[col] : col));
								lastCol = col;
							}
							g.doSetPixel(cx + x,y + (ph - 1 - cy));
						}
						data += colBytes;
					}
					data = oldData - ((w * colBytes) + pad);
				}
			}
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
		else
			_dataSize = _infoHeader->sizeImage;
		_data = new uint8_t[_dataSize];
		try {
			f.read(_data,sizeof(uint8_t),_dataSize);
		}
		catch(io_exception &e) {
			throw img_load_error(filename + ": Unable to read image-data: " + e.what());
		}
	}
}
