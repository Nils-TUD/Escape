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

#include <assert.h>
#include <common.h>
#include <stdarg.h>
#include <string.h>
#include <video.h>

void Video::move() {
	/* last line? */
	if(row >= VID_ROWS) {
		/* copy all chars one line back */
		copyScrToScr(screen(),(uint64_t*)screen() + VID_MAX_COLS,VID_ROWS - 1);
		/* clear last line */
		uint64_t *scr = (uint64_t*)screen() + VID_MAX_COLS * (VID_ROWS - 1);
		for(size_t x = 0; x < VID_COLS; x++)
			*scr++ = 0;
		row--;
	}
}

void Video::copyScrToScr(void *dst,const void *src,size_t rows) {
	uint64_t *idst = (uint64_t*)dst;
	uint64_t *isrc = (uint64_t*)src;
	for(size_t y = 0; y < rows; y++) {
		for(size_t x = 0; x < VID_COLS; x++)
			*idst++ = *isrc++;
		idst += VID_MAX_COLS - VID_COLS;
		isrc += VID_MAX_COLS - VID_COLS;
	}
}

void Video::copyScrToMem(void *dst,const void *src,size_t rows) {
	uint16_t *idst = (uint16_t*)dst;
	uint64_t *isrc = (uint64_t*)src;
	for(size_t y = 0; y < rows; y++) {
		for(size_t x = 0; x < VID_COLS; x++)
			*idst++ = *isrc++;
		isrc += VID_MAX_COLS - VID_COLS;
	}
}

void Video::copyMemToScr(void *dst,const void *src,size_t rows) {
	uint64_t *idst = (uint64_t*)dst;
	uint16_t *isrc = (uint16_t*)src;
	for(size_t y = 0; y < rows; y++) {
		for(size_t x = 0; x < VID_COLS; x++)
			*idst++ = *isrc++;
		idst += VID_MAX_COLS - VID_COLS;
	}
}

void Video::clear() {
	uint64_t *scr = (uint64_t*)screen();
	for(size_t y = 0; y < VID_ROWS; y++) {
		for(size_t x = 0; x < VID_COLS; x++)
			*scr++ = 0;
		scr += VID_MAX_COLS - VID_COLS;
	}
}
