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

#include <sys/common.h>
#include <sys/video.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

void Video::move() {
	/* last line? */
	if(row >= VID_ROWS) {
		size_t x;
		/* copy all chars one line back */
		copyScrToScr(screen(),(uint64_t*)screen() + MAX_COLS,VID_ROWS - 1);
		/* clear last line */
		uint64_t *scr = (uint64_t*)screen() + MAX_COLS * (VID_ROWS - 1);
		for(x = 0; x < VID_COLS; x++)
			*scr++ = 0;
		row--;
	}
}

void Video::copyScrToScr(void *dst,const void *src,size_t rows) {
	size_t x,y;
	uint64_t *idst = (uint64_t*)dst;
	uint64_t *isrc = (uint64_t*)src;
	for(y = 0; y < rows; y++) {
		for(x = 0; x < VID_COLS; x++)
			*idst++ = *isrc++;
		idst += MAX_COLS - VID_COLS;
		isrc += MAX_COLS - VID_COLS;
	}
}

void Video::copyScrToMem(void *dst,const void *src,size_t rows) {
	size_t x,y;
	uint16_t *idst = (uint16_t*)dst;
	uint64_t *isrc = (uint64_t*)src;
	for(y = 0; y < rows; y++) {
		for(x = 0; x < VID_COLS; x++)
			*idst++ = *isrc++;
		isrc += MAX_COLS - VID_COLS;
	}
}

void Video::copyMemToScr(void *dst,const void *src,size_t rows) {
	size_t x,y;
	uint64_t *idst = (uint64_t*)dst;
	uint16_t *isrc = (uint16_t*)src;
	for(y = 0; y < rows; y++) {
		for(x = 0; x < VID_COLS; x++)
			*idst++ = *isrc++;
		idst += MAX_COLS - VID_COLS;
	}
}

void Video::clear() {
	size_t x,y;
	uint64_t *scr = (uint64_t*)screen();
	for(y = 0; y < VID_ROWS; y++) {
		for(x = 0; x < VID_COLS; x++)
			*scr++ = 0;
		scr += MAX_COLS - VID_COLS;
	}
}
