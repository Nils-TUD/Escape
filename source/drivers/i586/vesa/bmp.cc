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
#include <esc/io.h>
#include <esc/dir.h>
#include <assert.h>
#include <stdlib.h>
#include "bmp.h"

// TODO use the C++ bitmap library

void bmp_draw(sVESAScreen *scr,sBitmap *bmp,gpos_t x,gpos_t y,fSetPixel func) {
	uint8_t *data = (uint8_t*)bmp->data;
	gpos_t w = bmp->infoHeader->width, h = bmp->infoHeader->height;
	gpos_t rw = w;
	size_t bpp = scr->mode->bitsPerPixel / 8;
	/* we assume RGB 24bit here */
	assert(bmp->infoHeader->compression == BI_RGB);
	assert(bmp->infoHeader->bitCount == 24);

	/* ensure that we stay within bounds */
	if((gsize_t)x + w > scr->mode->width)
		w = scr->mode->width - x;

	for(gpos_t cy = 0; cy < h; cy++) {
		uint8_t *workdata = data;
		for(gpos_t cx = 0; cx < w; cx++) {
			if((workdata[2] << 16 | workdata[1] << 8 | workdata[0]) != TRANSPARENCY) {
				gpos_t ypos = y + (h - cy - 1);
				if((gsize_t)ypos < scr->mode->height) {
					uint8_t *pos = scr->frmbuf + (ypos * scr->mode->width + cx + x) * bpp;
					func(*scr->mode,pos,workdata);
				}
			}
			workdata += 3;
		}
		data += 3 * rw;
		/* lines are 4-byte aligned */
		gpos_t rem = (rw * 3) % 4;
		if(rem)
			data += 4 - rem;
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
	if(IGNSIGS(read(fd,header,headerSize)) != (ssize_t)headerSize) {
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
		res = IGNSIGS(read(fd,bmp->colorTable,bmp->tableSize * sizeof(uint32_t)));
		if(res != (ssize_t)(bmp->tableSize * sizeof(uint32_t))) {
			printe("Invalid image '%s'",filename);
			goto errorColorTbl;
		}
	}

	/* now read the data */
	if(bmp->infoHeader->compression == BI_RGB) {
		size_t bytesPerLine;
		bytesPerLine = bmp->infoHeader->width * (bmp->infoHeader->bitCount / 8);
		bytesPerLine = ROUND_UP(bytesPerLine,sizeof(uint32_t));
		bmp->dataSize = bytesPerLine * bmp->infoHeader->height;
	}
	else
		bmp->dataSize = bmp->infoHeader->sizeImage;
	bmp->data = malloc(bmp->dataSize);
	if(bmp->data == NULL)
		goto errorColorTbl;
	if(seek(fd,bmp->fileHeader->dataOffset,SEEK_SET) < 0) {
		printe("Unable to seek to %zu\n",bmp->fileHeader->dataOffset);
		goto errorData;
	}
	res = IGNSIGS(read(fd,bmp->data,bmp->dataSize));
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
