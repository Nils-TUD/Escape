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

#ifndef BMP_H_
#define BMP_H_

#include <esc/common.h>
#include "vesa.h"

/* compression-modes */
#define BI_RGB			0	/* uncompressed */
#define BI_RLE8			1	/* valid if bitCount == 8 and height > 0 */
#define BI_RLE4			2	/* valid if bitCount == 4 and height > 0 */
#define BI_BITFIELDS	3	/* valid for bitCount == 16 or 32. uncompressed with colormasks */

#define TRANSPARENCY	0x00FF00FF

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
} A_PACKED sBMInfoHeader;

typedef struct {
	sBMFileHeader *fileHeader;
	sBMInfoHeader *infoHeader;
	uint32_t *colorTable;
	size_t tableSize;
	void *data;
	size_t dataSize;
} sBitmap;

/**
 * Draws the given bitmap at given position
 *
 * @param bmp the bitmap
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @param func the function to set a pixel
 */
void bmp_draw(sBitmap *bmp,gpos_t x,gpos_t y,fSetPixel func);

/**
 * Loads a bitmap from given file
 *
 * @param filename the filename (may be relative)
 * @return the bitmap or NULL if failed
 */
sBitmap *bmp_loadFromFile(const char *filename);

/**
 * Destroyes the given bitmap (free's mem)
 *
 * @param bmp the bitmap
 */
void bmp_destroy(sBitmap *bmp);

#endif /* BMP_H_ */
