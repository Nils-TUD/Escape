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

#pragma once

#define VID_COLS				80
#define VID_ROWS				30
#define BYTES_PER_COL			4
#define MAX_COLS				128

inline void *Video::screen() {
	return (void*)0xF0100000;
}

inline void Video::drawChar(ushort col,ushort row,char c) {
	uint32_t *video = (uint32_t*)screen() + row * MAX_COLS + col;
	*video = color << 8 | (unsigned char)c;
}
