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
#include <gui/graphics/graphics.h>
#include <gui/window.h>
#include <algorithm>
#include <string>
#include <iostream>
#include <string.h>

namespace gui {
	void Graphics::moveRows(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int up) {
		gsize_t bwidth = _buf->getWidth();
		gsize_t bheight = _buf->getHeight();
		gsize_t psize = _buf->getColorDepth() / 8;
		gsize_t wsize = width * psize;
		gsize_t bwsize = bwidth * psize;
		uint8_t *pixels = _buf->getBuffer();
		if(!validateParams(x,y,width,height))
			return;

		gpos_t startx = _offx + x;
		gpos_t starty = _offy + y;
		if(up > 0) {
			if(y < up)
				up = y;
		}
		else {
			if(starty + height - up > bheight)
				up = -(bheight - (height + starty));
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

	void Graphics::moveCols(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int left) {
		gsize_t bwidth = _buf->getWidth();
		gsize_t psize = _buf->getColorDepth() / 8;
		gsize_t wsize = width * psize;
		gsize_t bwsize = bwidth * psize;
		uint8_t *pixels = _buf->getBuffer();
		if(!validateParams(x,y,width,height))
			return;

		gpos_t startx = _offx + x;
		gpos_t starty = _offy + y;
		if(left > 0) {
			if(x < left)
				left = x;
		}
		else {
			if(startx + width - left > bwidth)
				left = -(bwidth - (width + startx));
		}

		pixels += startx * psize;
		for(gsize_t i = 0; i < height; i++) {
			memmove(pixels + (starty + i) * bwsize - left * psize,
				pixels + (starty + i) * bwsize,
				wsize);
		}
		updateMinMax(x - left,y);
		updateMinMax(x + width - left - 1,height - 1);
	}

	void Graphics::drawChar(gpos_t x,gpos_t y,char c) {
		gpos_t orgx = x;
		gpos_t orgy = y;
		gsize_t width = _font.getWidth();
		gsize_t height = _font.getHeight();
		if(!validateParams(x,y,width,height))
			return;

		updateMinMax(x,y);
		updateMinMax(x + width - 1,y + height - 1);
		gpos_t cx,cy;
		gpos_t xoff = x - orgx,yoff = y - orgy;
		gsize_t xend = xoff + width;
		gsize_t yend = yoff + height;
		for(cy = yoff; cy < yend; cy++) {
			for(cx = xoff; cx < xend; cx++) {
				if(_font.isPixelSet(c,cx,cy))
					doSetPixel(orgx + cx,orgy + cy);
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

		if(!validateLine(x0,y0,xn,yn))
			return;
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
		doSetPixel(*px,*py);
	}

	void Graphics::drawVertLine(gpos_t x,gpos_t y1,gpos_t y2) {
		if(!validateLine(x,y1,x,y2))
			return;
		updateMinMax(x,y1);
		updateMinMax(x,y2);
		if(y1 > y2)
			std::swap(y1,y2);
		for(; y1 <= y2; y1++)
			doSetPixel(x,y1);
	}

	void Graphics::drawHorLine(gpos_t y,gpos_t x1,gpos_t x2) {
		if(!validateLine(x1,y,x2,y))
			return;
		updateMinMax(x1,y);
		updateMinMax(x2,y);
		if(x1 > x2)
			std::swap(x1,x2);
		for(; x1 <= x2; x1++)
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
		if(!validateParams(x,y,width,height))
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
		gpos_t x = _minx;
		gpos_t y = _miny;
		gsize_t width = _maxx - _minx + 1;
		gsize_t height = _maxy - _miny + 1;
		if(!validateParams(x,y,width,height))
			return;

		_buf->requestUpdate(_offx + x,_offy + y,width,height);
		// reset region
		_minx = _width - 1;
		_maxx = 0;
		_miny = _height - 1;
		_maxy = 0;
	}

	void Graphics::update(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		if(!validateParams(x,y,width,height))
			return;

		_buf->update(x,y,width,height);
	}

	void Graphics::setSize(gpos_t x,gpos_t y,gsize_t width,gsize_t height,
			gsize_t pwidth,gsize_t pheight) {
		_width = getDim(x,width,pwidth);
		_width = getDim(_offx,_width,_buf->getWidth());
		_height = getDim(y,height,pheight);
		_height = getDim(_offy,_height,_buf->getHeight());
	}

	gsize_t Graphics::getDim(gpos_t off,gsize_t size,gsize_t max) {
		if(off + size > max) {
			if(off < max)
				return max - off;
			return 0;
		}
		return size;
	}

	bool Graphics::validateLine(gpos_t &x1,gpos_t &y1,gpos_t &x2,gpos_t &y2) {
		if(_width == 0 || _height == 0)
			return false;

		// TODO later we should calculate with sin&cos the end-position in bounds so that
		// the line will just be shorter and doesn't change the angle

		// ensure that none of the params reference the same variable. otherwise we can't detect
		// that lines are not visible in all cases (because the first one will detect it but also
		// change the variable and therefore the second won't detect it so that one point will be
		// considered visible)
		gpos_t xx1 = x1, xx2 = x2, yy1 = y1, yy2 = y2;
		int p1 = validatePoint(xx1,yy1);
		int p2 = validatePoint(xx2,yy2);
		if((p1 & OUT_LEFT) && (p2 & OUT_LEFT))
			return false;
		if((p1 & OUT_TOP) && (p2 & OUT_TOP))
			return false;
		if((p1 & OUT_RIGHT) && (p2 & OUT_RIGHT))
			return false;
		if((p1 & OUT_BOTTOM) && (p2 & OUT_BOTTOM))
			return false;
		// update the values (they're only needed when true is returned)
		x1 = xx1;
		x2 = xx2;
		y1 = yy1;
		y2 = yy2;
		return true;
	}

	int Graphics::validatePoint(gpos_t &x,gpos_t &y) {
		int res = 0;
		gpos_t minx = _minoffx - _offx;
		gpos_t miny = _minoffy - _offy;
		if(x < minx) {
			x = minx;
			res |= OUT_LEFT;
		}
		if(y < miny) {
			y = miny;
			res |= OUT_TOP;
		}
		if(_offx + x < 0) {
			x = -_offx;
			res |= OUT_LEFT;
		}
		if(_offy + y < 0) {
			y = -_offy;
			res |= OUT_TOP;
		}
		if(x >= _width) {
			x = _width - 1;
			res |= OUT_RIGHT;
		}
		if(y >= _height) {
			y = _height - 1;
			res |= OUT_BOTTOM;
		}
		return res;
	}

	bool Graphics::validateParams(gpos_t &x,gpos_t &y,gsize_t &width,gsize_t &height) {
		if(_width == 0 || _height == 0)
			return false;

		gpos_t minx = _minoffx - _offx;
		gpos_t miny = _minoffy - _offy;
		if(x < minx) {
			if(width < minx - x)
				return false;
			width -= minx - x;
			x = minx;
		}
		if(y < miny) {
			if(height < miny - y)
				return false;
			height -= miny - y;
			y = miny;
		}
		if(_offy + y < 0) {
			if(height < -(_offy + y))
				return false;
			height += _offy + y;
			y = -_offy;
		}
		if(_offx + x < 0) {
			if(width < -(_offx + x))
				return false;
			width += _offx + x;
			x = -_offx;
		}
		if(x + width > _width)
			width = max(0,(gpos_t)_width - x);
		if(y + height > _height)
			height = max(0,(gpos_t)_height - y);
		return width > 0 && height > 0;
	}
}
