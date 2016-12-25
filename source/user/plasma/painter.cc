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

#include <sys/common.h>
#include <esc/util.h>

#include "painter.h"
#include "math.h"

void Painter::resize(gsize_t width,gsize_t height) {
	this->width = width;
	this->height = height;
	delete[] x1vals;
	delete[] x2vals;
	delete[] y1vals;
	delete[] y2vals;
	x1vals = new float[width];
	x2vals = new float[width];
	y1vals = new float[height];
	y2vals = new float[height];
}

float Painter::getValue(uint ix,uint iy,float x,float y,float time) {
	// return Util::abs(sin(sin(x * 10 + time) + sin(y * 10 + time) + time));
	float v = 0;
	v += x1vals[ix];
	v += y2vals[iy];
	v += fastsin((x * 10 + y * 10 + time) / 2);
	float cx = x2vals[ix];
	float cy = y2vals[iy];
	v += fastsin(fastsqrt(100 * (cx + cy * cy) + 1) + time);
	v = v * PI_HALF;
	return esc::Util::abs(v);
}

void Painter::paintPlasma(const RGB &add,size_t factor,float time) {
	uint w = (width + factor - 1) / factor;
	uint h = (height + factor - 1) / factor;

	float sintime = .5f * fastsin(time / 5);
	float costime = .5f * fastcos(time / 3);

	for(uint x = 0; x < w; ++x) {
		float dx = (float)x / w;
		x1vals[x] = fastsin((dx * 10 + time));
		x2vals[x] = (x1vals[x] + sintime) * (x1vals[x] + sintime);
	}
	for(uint y = 0; y < h; ++y) {
		float dy = (float)y / h;
		y1vals[y] = fastsin((dy * 10 + time) / 2);
		y2vals[y] = y1vals[y] + costime;
	}

	float dx, dy = 0;
	float xadd = 1. / w;
	float yadd = 1. / h;
	for(uint y = 0; y < h; ++y, dy += yadd) {
		dx = 0;
		for(uint x = 0; x < w; ++x, dx += xadd) {
			float val = getValue(x,y,dx,dy,time);
			set(add,x,y,factor,val);
		}
	}
}
