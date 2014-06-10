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

#include <esc/common.h>
#include <gui/image/image.h>
#include <gui/graphics/graphics.h>
#include <string>

namespace gui {
	class BitmapImage : public Image {
	private:
		/* compression-modes */
		static const uint BI_RGB			= 0;	/* uncompressed */
		static const uint BI_RLE8			= 1;	/* valid if bitCount == 8 and height > 0 */
		static const uint BI_RLE4			= 2;	/* valid if bitCount == 4 and height > 0 */
		static const uint BI_BITFIELDS		= 3;	/* valid for bitCount == 16 or 32. uncompressed
													   with colormasks */

		static const uint32_t TRANSPARENT	= 0x00FF00FF;

		/* the file-header */
		typedef struct {
			/* "BM" */
			char type[2];
			/* size of the file (not reliable) */
			uint32_t size;
			/* reserved */
			uint32_t : 32;
			/* offset of the data */
			uint32_t dataOffset;
		} A_PACKED sBMFileHeader;

		/* the information-header */
		typedef struct {
			/* size of this struct */
			uint32_t size;
			/* size of the image */
			uint32_t width;
			uint32_t height;
			/* not used for BMP */
			uint16_t planes;
			/* color-depth in bpp (1, 4, 8, 16, 24 or 32); 1, 4 and 8 uses indexed colors */
			uint16_t bitCount;
			/* see BI_* */
			uint32_t compression;
			/* if compression == BI_RGB: 0 or the size of the image-data */
			/* otherwise: size of image-data */
			uint32_t sizeImage;
			/* horizontal/vertical resolution of the target-output-device in pixel per meter */
			/* 0 in most cases */
			uint32_t xPelsPerMeter;
			uint32_t yPelsPerMeter;
			/* if bitCount == 1: 0
			 * if bitCount == 4 or 8: number of entries in color-table; 0 = max (16 or 256)
			 * otherwise: number of entries in color-table; 0 = no color-table */
			uint32_t colorsUsed;
			/* if bitCount == 1, 4 or 8: nnumber of colors used in the image; 0 = all colors in
			 * 							 color-table
			 * otherwise: if color-table available, the number of them; otherwise 0 */
			uint32_t colorsImportant;
			uint32_t redmask;
			uint32_t greenmask;
			uint32_t bluemask;
			uint32_t alphamask;
		} A_PACKED sBMInfoHeader;

	public:
		BitmapImage(const std::string &filename)
			: Image(), _fileHeader(nullptr), _infoHeader(nullptr), _colorTable(nullptr), _tableSize(0),
				_data(nullptr), _dataSize(0) {
			loadFromFile(filename);
		}
		virtual ~BitmapImage() {
			delete[] _fileHeader;
			// don't delete _infoHeader since it's in the heap-area of fileHeader
			delete[] _colorTable;
			delete[] _data;
		}

		BitmapImage(const BitmapImage &img)
			: Image(img), _fileHeader(nullptr), _infoHeader(nullptr), _colorTable(nullptr),
			  _tableSize(0), _data(nullptr), _dataSize(0) {
			clone(img);
		}
		BitmapImage &operator=(const BitmapImage &img) {
			if(&img == this)
				return *this;
			Image::operator=(img);
			delete[] _fileHeader;
			delete[] _colorTable;
			delete[] _data;
			clone(img);
			return *this;
		}

		virtual Size getSize() const {
			return Size(_infoHeader->width,_infoHeader->height);
		}
		void paint(Graphics &g,const Pos &pos);

	private:
		void clone(const BitmapImage &img);
		void loadFromFile(const std::string &filename);
		void paintRGB(Graphics &g,gpos_t x,gpos_t y);
		void paintBitfields(Graphics &g,gpos_t x,gpos_t y);
		void paintPixel(Graphics &g,gpos_t x,gpos_t y,const Color &col,Color &lastCol);
		void paintPixel(Graphics &g,gpos_t x,gpos_t y,uint32_t col,uint32_t &lastCol);
		void paintPixel8(Graphics &g,gpos_t x,gpos_t y,uint32_t col,uint32_t &lastCol);
		static uint getShift(uint32_t val);

		sBMFileHeader *_fileHeader;
		sBMInfoHeader *_infoHeader;
		uint32_t *_colorTable;
		size_t _tableSize;
		uint8_t *_data;
		size_t _dataSize;
	};
}
