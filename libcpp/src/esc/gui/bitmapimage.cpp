/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/stream.h>
#include <esc/gui/common.h>
#include <esc/gui/bitmapimage.h>

namespace esc {
	namespace gui {
		void BitmapImage::paint(Graphics &g,tCoord x,tCoord y) {
			if(_data == NULL)
				return;
			switch(_infoHeader->compression) {
				case BI_RGB:
					switch(_infoHeader->bitCount) {
						case 1:
						case 4:
						case 8:
						case 24: {
							u32 bitCount = _infoHeader->bitCount;
							u8 *data = _data;
							tSize w = _infoHeader->width, h = _infoHeader->height;
							tCoord cx,cy;
							u32 lastCol = 0;
							g.setColor(Color(0));
							g.updateMinMax(x,y);
							g.updateMinMax(x + w,y + h);
							for(cy = 0; cy < h; cy++) {
								for(cx = 0; cx < w; cx++) {
									// TODO performance might be improvable
									u32 col = bitCount <= 8 ? *data : *(u32*)data;
									if(col != lastCol) {
										g.setColor(Color(bitCount <= 8 ? _colorTable[col] : col));
										lastCol = col;
									}
									g.doSetPixel(cx + x,y + (h - cy));
									if(bitCount <= 8)
										data++;
									else
										data += 3;
								}
								// lines are 4-byte aligned
								cx = (cx * (bitCount / 8)) % 4;
								if(cx)
									data += 4 - cx;
							}
						}
						break;
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

		void BitmapImage::loadFromFile(const String &filename) {
			// read header
			u32 headerSize = sizeof(sBMFileHeader) + sizeof(sBMInfoHeader);
			u8 *header = new u8[headerSize];
			FileStream f(filename.c_str(),FileStream::READ);
			if(f.read(header,headerSize) != (s32)headerSize) {
				// TODO throw exception
				printe("Invalid image '%s'",filename.c_str());
				return;
			}

			_fileHeader = (sBMFileHeader*)header;
			_infoHeader = (sBMInfoHeader*)(_fileHeader + 1);

			// check header-type
			if(_fileHeader->type[0] != 'B' || _fileHeader->type[1] != 'M') {
				// TODO throw exception
				printe("Invalid image '%s'",filename.c_str());
				return;
			}

			// determine color-table-size
			_tableSize = 0;
			if(_infoHeader->colorsUsed == 0) {
				if(_infoHeader->bitCount == 1 || _infoHeader->bitCount == 4 ||
						_infoHeader->bitCount == 8)
					_tableSize = 1 << _infoHeader->bitCount;
			}
			else
				_tableSize = _infoHeader->colorsUsed;

			// read color-table, if present
			if(_tableSize > 0) {
				_colorTable = new u32[_tableSize];
				if(f.read(_colorTable,_tableSize * sizeof(u32)) != (s32)(_tableSize * sizeof(u32))) {
					// TODO throw exception
					printe("Invalid image '%s'",filename.c_str());
					return;
				}
			}

			// now read the data
			if(_infoHeader->compression == BI_RGB) {
				u32 bytesPerLine;
				bytesPerLine = _infoHeader->width * (_infoHeader->bitCount / 8);
				bytesPerLine = (bytesPerLine + sizeof(u32) - 1) & ~(sizeof(u32) - 1);
				_dataSize = bytesPerLine * _infoHeader->height;
			}
			else
				_dataSize = _infoHeader->sizeImage;
			_data = new u8[_dataSize];
			if(f.read(_data,_dataSize * sizeof(u8)) != (s32)(_dataSize * sizeof(u8))) {
				// TODO throw exception
				printe("Invalid image '%s'",filename.c_str());
				return;
			}
		}
	}
}
