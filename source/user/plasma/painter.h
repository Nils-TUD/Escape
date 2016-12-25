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

#include "math.h"

class Painter {
public:
	explicit Painter()
		: width(), height(), x1vals(), x2vals(), y1vals(), y2vals() {
	}

	virtual void paint(const RGB &add,size_t factor,float time) = 0;

	void paintPlasma(const RGB &add,size_t factor,float time);

	void resize(gsize_t width,gsize_t height);

protected:
	virtual void set(const RGB &add,gpos_t x,gpos_t y,size_t factor,float val) = 0;

	float getValue(uint ix,uint iy,float x,float y,float time);

	gsize_t width;
	gsize_t height;
	float *x1vals;
	float *x2vals;
	float *y1vals;
	float *y2vals;
};
