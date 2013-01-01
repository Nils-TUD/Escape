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

using namespace std;

namespace gui {
	void Graphics::moveRows(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int up) {
		Size bsize = _buf->getSize();
		gsize_t psize = _buf->getColorDepth() / 8;
		gsize_t wsize = width * psize;
		gsize_t bwsize = bsize.width * psize;
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
			if(starty + height - up > bsize.height)
				up = -(bsize.height - (height + starty));
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
		Size bsize = _buf->getSize();
		gsize_t psize = _buf->getColorDepth() / 8;
		gsize_t wsize = width * psize;
		gsize_t bwsize = bsize.width * psize;
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
			if(startx + width - left > bsize.width)
				left = -(bsize.width - (width + startx));
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
		Size fsize = _font.getSize();
		if(!validateParams(x,y,fsize.width,fsize.height))
			return;

		updateMinMax(x,y);
		updateMinMax(x + fsize.width - 1,y + fsize.height - 1);
		gpos_t cx,cy;
		gpos_t xoff = x - orgx,yoff = y - orgy;
		gsize_t xend = xoff + fsize.width;
		gsize_t yend = yoff + fsize.height;
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
		size_t charWidth = _font.getSize().width;
		size_t end = start + MIN(str.length(),count);
		for(size_t i = start; i < end; i++) {
			drawChar(x,y,str[i]);
			x += charWidth;
		}
	}

	void Graphics::drawLine(gpos_t x0,gpos_t y0,gpos_t xn,gpos_t yn) {
		if(_size.empty())
			return;
		if(x0 == xn) {
			drawVertLine(x0,y0,yn);
			return;
		}
		if(y0 == yn) {
			drawHorLine(y0,x0,xn);
			return;
		}

		// default settings
		gpos_t x = x0;
		gpos_t y = y0;
		gpos_t *px = &x;
		gpos_t *py = &y;
		int dx = xn - x0;
		int dy = yn - y0;
		int incx = 1, incy = 1;

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
			swap(x,y);
			swap(px,py);
			swap(dx,dy);
			swap(x0,y0);
			swap(xn,yn);
			swap(incx,incy);
		}

		int d = 2 * dy - dx;
		/*Increments E & NE*/
		int incrE = 2 * dy;
		int incrNE = 2 * (dy - dx);

		gpos_t minx = _minoffx - _offx;
		gpos_t miny = _minoffy - _offy;
		gpos_t maxx = _size.width - 1;
		gpos_t maxy = _size.height - 1;
		for(x = x0; x != xn; x += incx) {
			setLinePixel(minx,miny,maxx,maxy,*px,*py);
			if(d < 0)
				d += incrE;
			else {
				d += incrNE;
				y += incy;
			}
		}
		setLinePixel(minx,miny,maxx,maxy,*px,*py);
		updateMinMax(max(minx,min(maxx,x0)),max(miny,min(maxy,y0)));
		updateMinMax(max(minx,min(maxx,xn)),max(miny,min(maxy,yn)));
	}

	void Graphics::drawVertLine(gpos_t x,gpos_t y1,gpos_t y2) {
		if(!validateLine(x,y1,x,y2))
			return;
		updateMinMax(x,y1);
		updateMinMax(x,y2);
		if(y1 > y2)
			swap(y1,y2);
		for(; y1 <= y2; y1++)
			doSetPixel(x,y1);
	}

	void Graphics::drawHorLine(gpos_t y,gpos_t x1,gpos_t x2) {
		if(!validateLine(x1,y,x2,y))
			return;
		updateMinMax(x1,y);
		updateMinMax(x2,y);
		if(x1 > x2)
			swap(x1,x2);
		for(; x1 <= x2; x1++)
			doSetPixel(x1,y);
	}

	void Graphics::drawRect(gpos_t x,gpos_t y,const Size &size) {
		if(size.empty())
			return;

		// top
		drawHorLine(y,x,x + size.width - 1);
		// right
		drawVertLine(x + size.width - 1,y,y + size.height - 1);
		// bottom
		drawHorLine(y + size.height - 1,x,x + size.width - 1);
		// left
		drawVertLine(x,y,y + size.height - 1);
	}

	void Graphics::fillRect(gpos_t x,gpos_t y,const Size &size) {
		Size rsize = size;
		if(!validateParams(x,y,rsize.width,rsize.height))
			return;

		gpos_t yend = y + rsize.height;
		updateMinMax(x,y);
		updateMinMax(x + rsize.width - 1,yend - 1);
		gpos_t xcur;
		gpos_t xend = x + rsize.width;
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

		_buf->requestUpdate(_offx + x,_offy + y,Size(width,height));
		// reset region
		_minx = _size.width - 1;
		_maxx = 0;
		_miny = _size.height - 1;
		_maxy = 0;
	}

	void Graphics::update(gpos_t x,gpos_t y,const Size &size) {
		Size rsize = size;
		if(!validateParams(x,y,rsize.width,rsize.height))
			return;

		_buf->update(x,y,rsize);
	}

	void Graphics::setSize(gpos_t x,gpos_t y,const Size &size,const Size &psize) {
		_size.width = getDim(x,size.width,psize.width);
		_size.width = getDim(_offx,_size.width,_buf->getSize().width);
		_size.height = getDim(y,size.height,psize.height);
		_size.height = getDim(_offy,_size.height,_buf->getSize().height);
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
		if(_size.empty())
			return false;

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
		if(x >= _size.width) {
			x = _size.width - 1;
			res |= OUT_RIGHT;
		}
		if(y >= _size.height) {
			y = _size.height - 1;
			res |= OUT_BOTTOM;
		}
		return res;
	}

	bool Graphics::validateParams(gpos_t &x,gpos_t &y,gsize_t &width,gsize_t &height) {
		if(_size.empty())
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
		if(x + width > _size.width)
			width = max(0,(gpos_t)_size.width - x);
		if(y + height > _size.height)
			height = max(0,(gpos_t)_size.height - y);
		return width > 0 && height > 0;
	}
}
