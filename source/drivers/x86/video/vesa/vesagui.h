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

#include <sys/common.h>

#include "image.h"
#include "vesascreen.h"

class VESAGUI {
public:
	explicit VESAGUI();

	void setCursor(VESAScreen *scr,void *shmem,gpos_t newCurX,gpos_t newCurY,int newCursor);
	void update(VESAScreen *scr,void *shmem,gpos_t x,gpos_t y,gsize_t width,gsize_t height);

private:
	void doSetCursor(VESAScreen *scr,void *shmem,gpos_t x,gpos_t y,int newCursor);
	void copyRegion(VESAScreen *scr,uint8_t *src,uint8_t *dst,gsize_t width,gsize_t height,
		gpos_t x1,gpos_t y1,gpos_t x2,gpos_t y2,gsize_t w1,gsize_t w2,gsize_t h1);

	uint8_t *_cursorCopy;
	gpos_t _lastX;
	gpos_t _lastY;
	uint8_t _curCursor;
	VESAImage *_cursor[7];
};
