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
#include <esc/io.h>
#include <esc/dir.h>
#include <assert.h>
#include <stdlib.h>
#include "bmp.h"
#include "vesa.h"

void bmp_draw(sBitmap *bmp,gpos_t x,gpos_t y,fSetPixel func) {
	uint8_t *data = (uint8_t*)bmp->data;
	gsize_t w = bmp->infoHeader->width, h = bmp->infoHeader->height;
	gpos_t cx,cy;
	/* we assume RGB 24bit here */
	assert(bmp->infoHeader->compression == BI_RGB);
	assert(bmp->infoHeader->bitCount == 24);
	for(cy = 0; cy < h; cy++) {
		for(cx = 0; cx < w; cx++) {
			uint32_t col = data[2] << 16 | data[1] << 8 | data[0];
			if(col != TRANSPARENCY)
				func(cx + x,y + (h - cy - 1),col);
			data += 3;
		}
		/* lines are 4-byte aligned */
		cx = (cx * 3) % 4;
		if(cx)
			data += 4 - cx;
	}
}

sBitmap *bmp_loadFromFile(const char *filename) {
	/* read header */
	int fd;
	ssize_t res;
	sBitmap *bmp = NULL;
	size_t headerSize = sizeof(sBMFileHeader) + sizeof(sBMInfoHeader);
	void *header = malloc(headerSize);
	if(header == NULL)
		return NULL;

	bmp = (sBitmap*)malloc(sizeof(sBitmap));
	if(bmp == NULL)
		goto errorHeader;

	fd = open(filename,IO_READ);
	if(fd < 0)
		goto errorBmp;
	if(RETRY(read(fd,header,headerSize)) != (ssize_t)headerSize) {
		printe("Invalid image '%s'",filename);
		goto errorClose;
	}

	bmp->fileHeader = (sBMFileHeader*)header;
	bmp->infoHeader = (sBMInfoHeader*)(bmp->fileHeader + 1);

	/* check header-type */
	if(bmp->fileHeader->type[0] != 'B' || bmp->fileHeader->type[1] != 'M') {
		printe("Invalid image '%s'",filename);
		goto errorClose;
	}

	/* determine color-table-size */
	bmp->tableSize = 0;
	if(bmp->infoHeader->colorsUsed == 0) {
		if(bmp->infoHeader->bitCount == 1 || bmp->infoHeader->bitCount == 4 ||
				bmp->infoHeader->bitCount == 8)
			bmp->tableSize = 1 << bmp->infoHeader->bitCount;
	}
	else
		bmp->tableSize = bmp->infoHeader->colorsUsed;

	/* read color-table, if present */
	if(bmp->tableSize > 0) {
		bmp->colorTable = (uint32_t*)malloc(bmp->tableSize * sizeof(uint32_t));
		if(bmp->colorTable == NULL)
			goto errorClose;
		res = RETRY(read(fd,bmp->colorTable,bmp->tableSize * sizeof(uint32_t)));
		if(res != (ssize_t)(bmp->tableSize * sizeof(uint32_t))) {
			printe("Invalid image '%s'",filename);
			goto errorColorTbl;
		}
	}

	/* now read the data */
	if(bmp->infoHeader->compression == BI_RGB) {
		size_t bytesPerLine;
		bytesPerLine = bmp->infoHeader->width * (bmp->infoHeader->bitCount / 8);
		bytesPerLine = (bytesPerLine + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1);
		bmp->dataSize = bytesPerLine * bmp->infoHeader->height;
	}
	else
		bmp->dataSize = bmp->infoHeader->sizeImage;
	bmp->data = malloc(bmp->dataSize);
	if(bmp->data == NULL)
		goto errorColorTbl;
	res = RETRY(read(fd,bmp->data,bmp->dataSize));
	if(res != (ssize_t)bmp->dataSize) {
		printe("Invalid image '%s'",filename);
		goto errorData;
	}
	close(fd);
	return bmp;

errorData:
	free(bmp->data);
errorColorTbl:
	free(bmp->colorTable);
errorClose:
	close(fd);
errorBmp:
	free(bmp);
errorHeader:
	free(header);
	return NULL;
}

void bmp_destroy(sBitmap *bmp) {
	free(bmp->data);
	free(bmp->colorTable);
	free(bmp->fileHeader);
	free(bmp);
}
