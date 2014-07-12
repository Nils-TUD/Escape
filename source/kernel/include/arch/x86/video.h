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

#include <mem/pagedir.h>
#include <string.h>

#define VID_COLS				80
#define VID_ROWS				25
#define BYTES_PER_COL			2

inline void *Video::screen() {
	return (void*)(KERNEL_START - KERNEL_P_ADDR + 0xB8000);
}

inline void Video::copyScrToScr(void *dst,const void *src,size_t rows) {
	memcpy(dst,src,rows * VID_COLS * 2);
}

inline void Video::copyScrToMem(void *dst,const void *src,size_t rows) {
	memcpy(dst,src,rows * VID_COLS * 2);
}

inline void Video::copyMemToScr(void *dst,const void *src,size_t rows) {
	memcpy(dst,src,rows * VID_COLS * 2);
}

inline void Video::drawChar(ushort col,ushort row,char c) {
	char *video = (char*)screen() + row * VID_COLS * 2 + col * 2;
	*video = c;
	video++;
	*video = color;
}
