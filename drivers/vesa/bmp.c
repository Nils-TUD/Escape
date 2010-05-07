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
#include <esc/io.h>
#include <esc/dir.h>
#include <assert.h>
#include <stdlib.h>
#include "bmp.h"
#include "vesa.h"

void bmp_draw(sBitmap *bmp,tCoord x,tCoord y,fSetPixel func) {
	u8 *data = bmp->data;
	tSize w = bmp->infoHeader->width, h = bmp->infoHeader->height;
	tCoord cx,cy;
	/* we assume RGB 24bit here */
	assert(bmp->infoHeader->compression == BI_RGB);
	assert(bmp->infoHeader->bitCount == 24);
	for(cy = 0; cy < h; cy++) {
		for(cx = 0; cx < w; cx++) {
			u32 col = data[2] << 16 | data[1] << 8 | data[0];
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
	char apath[MAX_PATH_LEN];
	tFD fd;
	sBitmap *bmp = NULL;
	u32 headerSize = sizeof(sBMFileHeader) + sizeof(sBMInfoHeader);
	u8 *header = malloc(headerSize);
	if(header == NULL)
		return NULL;

	bmp = (sBitmap*)malloc(sizeof(sBitmap));
	if(bmp == NULL)
		goto errorHeader;

	abspath(apath,sizeof(apath),filename);
	fd = open(apath,IO_READ);
	if(fd < 0)
		goto errorBmp;
	if(read(fd,header,headerSize) != (s32)headerSize) {
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
		bmp->colorTable = (u32*)malloc(bmp->tableSize * sizeof(u32));
		if(bmp->colorTable == NULL)
			goto errorClose;
		if(read(fd,bmp->colorTable,bmp->tableSize * sizeof(u32)) != (s32)(bmp->tableSize * sizeof(u32))) {
			printe("Invalid image '%s'",filename);
			goto errorColorTbl;
		}
	}

	/* now read the data */
	if(bmp->infoHeader->compression == BI_RGB) {
		u32 bytesPerLine;
		bytesPerLine = bmp->infoHeader->width * (bmp->infoHeader->bitCount / 8);
		bytesPerLine = (bytesPerLine + sizeof(u32) - 1) & ~(sizeof(u32) - 1);
		bmp->dataSize = bytesPerLine * bmp->infoHeader->height;
	}
	else
		bmp->dataSize = bmp->infoHeader->sizeImage;
	bmp->data = (u8*)malloc(bmp->dataSize);
	if(bmp->data == NULL)
		goto errorColorTbl;
	if(read(fd,bmp->data,bmp->dataSize * sizeof(u8)) != (s32)(bmp->dataSize * sizeof(u8))) {
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
