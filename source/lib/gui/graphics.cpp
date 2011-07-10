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
#include <esc/proc.h>
#include <esc/debug.h>
#include <gui/graphics.h>
#include <gui/window.h>
#include <algorithm>
#include <string>
#include <iostream>
#include <string.h>

namespace gui {
	void Graphics::moveLines(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int up) {
		gsize_t bwidth = _buf->getWidth();
		gsize_t bheight = _buf->getHeight();
		gsize_t psize = _buf->getColorDepth() / 8;
		gsize_t wsize = width * psize;
		gsize_t bwsize = bwidth * psize;
		uint8_t *pixels = _buf->getBuffer();
		validateParams(x,y,width,height,true);
		if(width == 0 || height == 0)
			return;

		gpos_t startx = _offx + x;
		gpos_t starty = _offy + y;
		if(up > 0) {
			if(y < up)
				up = y;
		}
		else {
			if(y + height - up > bheight)
				up = -(bheight - (height + y));
		}

		pixels += startx * psize;
		if(up > 0) {
			for(gsize_t i = 0; i < height; i++) {
				memcpy(pixels + (starty + i - up) * bwsize,
					pixels + (starty + i) * bwsize,
					wsize);
			}
		}
		else {
			for(gsize_t i = 0; i < height; i++) {
				memcpy(pixels + (starty + height - 1 - i - up) * bwsize,
					pixels + (starty + height - 1 - i) * bwsize,
					wsize);
			}
		}
		updateMinMax(x,y - up);
		updateMinMax(width - 1,y + height - up - 1);
	}

	void Graphics::drawChar(gpos_t x,gpos_t y,char c) {
		gsize_t width = _font.getWidth();
		gsize_t height = _font.getHeight();
		validateParams(x,y,width,height,true);
		updateMinMax(x,y);
		updateMinMax(x + width - 1,y + height - 1);
		gpos_t cx,cy;
		for(cy = 0; cy < height; cy++) {
			for(cx = 0; cx < width; cx++) {
				if(_font.isPixelSet(c,cx,cy))
					doSetPixel(x + cx,y + cy);
			}
		}
	}

	void Graphics::drawString(gpos_t x,gpos_t y,const string &str) {
		drawString(x,y,str,0,str.length());
	}

	void Graphics::drawString(gpos_t x,gpos_t y,const string &str,size_t start,size_t count) {
		size_t charWidth = _font.getWidth();
		size_t end = start + MIN(str.length(),count);
		for(size_t i = start; i < end; i++) {
			drawChar(x,y,str[i]);
			x += charWidth;
		}
	}

	void Graphics::drawLine(gpos_t x0,gpos_t y0,gpos_t xn,gpos_t yn) {
		int	dx,	dy,	d;
		int incrE, incrNE;	/*Increments for move to E	& NE*/
		gpos_t x,y;			/*Start & current pixel*/
		int incx, incy;
		gpos_t *px, *py;

		// TODO later we should calculate with sin&cos the end-position in bounds so that
		// the line will just be shorter and doesn't change the angle
		validatePos(x0,y0);
		validatePos(xn,yn);
		updateMinMax(x0,y0);
		updateMinMax(xn,yn);

		// default settings
		x = x0;
		y = y0;
		px = &x;
		py = &y;
		dx = xn - x0;
		dy = yn - y0;
		incx = incy = 1;

		// mirror by x-axis ?
		if(dx < 0) {
			dx = -dx;
			incx = -1;
		}
		// mirror by y-axis ?
		if(dy < 0) {
			dy = -dy;
			incy = -1;
		}
		// handle 45° < x < 90°
		if(dx < dy) {
			std::swap(x,y);
			std::swap(px,py);
			std::swap(dx,dy);
			std::swap(x0,y0);
			std::swap(xn,yn);
			std::swap(incx,incy);
		}

		d = 2 * dy - dx;
		/*Increments E & NE*/
		incrE = 2 * dy;
		incrNE = 2 * (dy - dx);

		for(x = x0; x != xn; x += incx) {
			doSetPixel(*px,*py);
			if(d < 0)
				d += incrE;
			else {
				d += incrNE;
				y += incy;
			}
		}
	}

	void Graphics::drawVertLine(gpos_t x,gpos_t y1,gpos_t y2) {
		if(y1 < 0)
			y1 = 0;
		if(y2 < 0)
			y2 = 0;
		validatePos(x,y1);
		validatePos(x,y2);
		updateMinMax(x,y1);
		updateMinMax(x,y2);
		if(y1 > y2)
			std::swap(y1,y2);
		for(; y1 < y2; y1++)
			doSetPixel(x,y1);
	}

	void Graphics::drawHorLine(gpos_t y,gpos_t x1,gpos_t x2) {
		if(x1 < 0)
			x1 = 0;
		if(x2 < 0)
			x2 = 0;
		validatePos(x1,y);
		validatePos(x2,y);
		updateMinMax(x1,y);
		updateMinMax(x2,y);
		if(x1 > x2)
			std::swap(x1,x2);
		for(; x1 < x2; x1++)
			doSetPixel(x1,y);
	}

	void Graphics::drawRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		if(width == 0 || height == 0)
			return;

		// top
		drawHorLine(y,x,x + width - 1);
		// right
		drawVertLine(x + width - 1,y,y + height - 1);
		// bottom
		drawHorLine(y + height - 1,x,x + width - 1);
		// left
		drawVertLine(x,y,y + height - 1);
	}

	void Graphics::fillRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		validateParams(x,y,width,height,true);
		if(width == 0 || height == 0)
			return;

		gpos_t yend = y + height;
		updateMinMax(x,y);
		updateMinMax(x + width - 1,yend - 1);
		gpos_t xcur;
		gpos_t xend = x + width;
		for(; y < yend; y++) {
			for(xcur = x; xcur < xend; xcur++)
				doSetPixel(xcur,y);
		}
	}

	void Graphics::requestUpdate() {
		_buf->requestUpdate(_offx + _minx,_offy + _miny,_maxx - _minx + 1,_maxy - _miny + 1);
		// reset region
		_minx = _buf->getWidth() - 1;
		_maxx = 0;
		_miny = _buf->getHeight() - 1;
		_maxy = 0;
	}

	void Graphics::update(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		validateParams(x,y,width,height,false);
		_buf->update(x,y,width,height);
	}

	void Graphics::validatePos(gpos_t &x,gpos_t &y) {
		gsize_t bwidth = _buf->getWidth();
		gsize_t bheight = _buf->getHeight();
		if(x < 0)
			x = 0;
		if(y < 0)
			y = 0;
		if(x >= bwidth - _offx)
			x = bwidth - _offx - 1;
		if(y >= bheight - _offy)
			y = bheight - _offy - 1;
	}

	void Graphics::validateParams(gpos_t &x,gpos_t &y,gsize_t &width,gsize_t &height,bool checkPos) {
		gsize_t bwidth = _buf->getWidth();
		gsize_t bheight = _buf->getHeight();
		if(checkPos) {
			if(x < 0)
				x = 0;
			if(y < 0)
				y = 0;
		}
		if(_offx + x + width > bwidth)
			width = MAX(0,(int)bwidth - x - _offx);
		if(_offy + y + height > bheight)
			height = MAX(0,(int)bheight - y - _offy);
	}
}
