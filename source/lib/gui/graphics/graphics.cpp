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
	void Graphics::moveRows(const Pos &pos,const Size &size,int up) {
		Size rsize = size;
		Pos rpos = pos;
		Size bsize = _buf->getSize();
		gsize_t psize = Application::getInstance()->getColorDepth() / 8;
		// TODO really size.width?
		gsize_t wsize = size.width * psize;
		gsize_t bwsize = bsize.width * psize;
		uint8_t *pixels = getPixels();
		if(!pixels || !validateParams(rpos,rsize))
			return;

		gpos_t startx = _off.x + rpos.x;
		gpos_t starty = _off.y + rpos.y;
		if(up > 0) {
			if(rpos.y < up)
				up = rpos.y;
		}
		else {
			if(starty + rsize.height - up > bsize.height)
				up = -(bsize.height - (rsize.height + starty));
		}

		pixels += startx * psize;
		if(up > 0) {
			for(gsize_t i = 0; i < rsize.height; i++) {
				memcpy(pixels + (starty + i - up) * bwsize,
					pixels + (starty + i) * bwsize,
					wsize);
			}
		}
		else {
			for(gsize_t i = 0; i < rsize.height; i++) {
				memcpy(pixels + (starty + rsize.height - 1 - i - up) * bwsize,
					pixels + (starty + rsize.height - 1 - i) * bwsize,
					wsize);
			}
		}
		updateMinMax(Pos(rpos.x,rpos.y - up));
		updateMinMax(Pos(rsize.width - 1,rpos.y + rsize.height - up - 1));
	}

	void Graphics::moveCols(const Pos &pos,const Size &size,int left) {
		Size rsize = size;
		Pos rpos = pos;
		Size bsize = _buf->getSize();
		gsize_t psize = Application::getInstance()->getColorDepth() / 8;
		gsize_t wsize = size.width * psize;
		gsize_t bwsize = bsize.width * psize;
		uint8_t *pixels = getPixels();
		if(!pixels || !validateParams(rpos,rsize))
			return;

		gpos_t startx = _off.x + rpos.x;
		gpos_t starty = _off.y + rpos.y;
		if(left > 0) {
			if(rpos.x < left)
				left = rpos.x;
		}
		else {
			if(startx + rsize.width - left > bsize.width)
				left = -(bsize.width - (rsize.width + startx));
		}

		pixels += startx * psize;
		for(gsize_t i = 0; i < rsize.height; i++) {
			memmove(pixels + (starty + i) * bwsize - left * psize,
				pixels + (starty + i) * bwsize,
				wsize);
		}
		updateMinMax(Pos(rpos.x - left,rpos.y));
		updateMinMax(Pos(rpos.x + rsize.width - left - 1,rsize.height - 1));
	}

	void Graphics::drawChar(const Pos &pos,char c) {
		Pos rpos = pos;
		Size fsize = _font.getSize();
		if(!getPixels() || !validateParams(rpos,fsize))
			return;

		updateMinMax(rpos);
		updateMinMax(Pos(rpos.x + fsize.width - 1,rpos.y + fsize.height - 1));
		gpos_t cx,cy;
		gpos_t xoff = rpos.x - pos.x,yoff = rpos.y - pos.y;
		gpos_t xend = xoff + fsize.width;
		gpos_t yend = yoff + fsize.height;
		for(cy = yoff; cy < yend; cy++) {
			for(cx = xoff; cx < xend; cx++) {
				if(_font.isPixelSet(c,cx,cy))
					doSetPixel(pos.x + cx,pos.y + cy);
			}
		}
	}

	void Graphics::drawString(const Pos &pos,const string &str) {
		drawString(pos,str,0,str.length());
	}

	void Graphics::drawString(const Pos &pos,const string &str,size_t start,size_t count) {
		Pos rpos = pos;
		size_t charWidth = _font.getSize().width;
		size_t end = start + min(str.length(),count);
		for(size_t i = start; i < end; i++) {
			drawChar(rpos,str[i]);
			rpos.x += charWidth;
		}
	}

	void Graphics::drawLine(const Pos &p0,const Pos &pn) {
		if(!getPixels() || _size.empty())
			return;
		if(p0.x == pn.x) {
			drawVertLine(p0.x,p0.y,pn.y);
			return;
		}
		if(p0.y == pn.y) {
			drawHorLine(p0.y,p0.x,pn.x);
			return;
		}

		// default settings
		gpos_t x0 = p0.x;
		gpos_t y0 = p0.y;
		gpos_t xn = pn.x;
		gpos_t yn = pn.y;
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

		gpos_t minx = _minoff.x - _off.x;
		gpos_t miny = _minoff.y - _off.y;
		gpos_t maxx = _size.width - 1;
		gpos_t maxy = _size.height - 1;
		for(x = x0; x != xn; x += incx) {
			setLinePixel(minx,miny,maxx,maxy,Pos(*px,*py));
			if(d < 0)
				d += incrE;
			else {
				d += incrNE;
				y += incy;
			}
		}
		setLinePixel(minx,miny,maxx,maxy,Pos(*px,*py));
		updateMinMax(Pos(max(minx,min(maxx,x0)),max(miny,min(maxy,y0))));
		updateMinMax(Pos(max(minx,min(maxx,xn)),max(miny,min(maxy,yn))));
	}

	void Graphics::drawVertLine(gpos_t x,gpos_t y1,gpos_t y2) {
		if(!getPixels() || !validateLine(x,y1,x,y2))
			return;
		updateMinMax(Pos(x,y1));
		updateMinMax(Pos(x,y2));
		if(y1 > y2)
			swap(y1,y2);
		for(; y1 <= y2; y1++)
			doSetPixel(x,y1);
	}

	void Graphics::drawHorLine(gpos_t y,gpos_t x1,gpos_t x2) {
		if(!getPixels() || !validateLine(x1,y,x2,y))
			return;
		updateMinMax(Pos(x1,y));
		updateMinMax(Pos(x2,y));
		if(x1 > x2)
			swap(x1,x2);
		for(; x1 <= x2; x1++)
			doSetPixel(x1,y);
	}

	void Graphics::drawRect(const Pos &pos,const Size &size) {
		if(!getPixels() || size.empty())
			return;

		// top
		drawHorLine(pos.y,pos.x,pos.x + size.width - 1);
		// right
		drawVertLine(pos.x + size.width - 1,pos.y,pos.y + size.height - 1);
		// bottom
		drawHorLine(pos.y + size.height - 1,pos.x,pos.x + size.width - 1);
		// left
		drawVertLine(pos.x,pos.y,pos.y + size.height - 1);
	}

	void Graphics::fillRect(const Pos &pos,const Size &size) {
		Pos rpos = pos;
		Size rsize = size;
		if(!getPixels() || !validateParams(rpos,rsize))
			return;

		gpos_t yend = rpos.y + rsize.height;
		updateMinMax(rpos);
		updateMinMax(Pos(rpos.x + rsize.width - 1,yend - 1));
		gpos_t xcur;
		gpos_t xend = rpos.x + rsize.width;

		gcoldepth_t bpp = Application::getInstance()->getColorDepth();
		gsize_t bwidth = _buf->getSize().width;
		uint8_t *orgaddr = getPixels() + (((_off.y + rpos.y) * bwidth + (_off.x + rpos.x)) * (bpp / 8));
		gsize_t widthadd = bwidth * (bpp / 8);
		// optimized versions for the individual color depths
		// This is necessary if we want to have reasonable speed because the simple version
		// performs too many function-calls (one to a virtual-function and one to memcpy
		// that the compiler doesn't inline). Additionally the offset into the
		// memory-region will be calculated many times.
		// This version is much quicker :)
		switch(bpp) {
			case 16: {
				for(; rpos.y < yend; rpos.y++) {
					uint16_t *addr = (uint16_t*)orgaddr;
					for(xcur = rpos.x; xcur < xend; xcur++)
						*addr++ = _col;
					orgaddr += widthadd;
				}
			}
			break;

			case 24: {
				uint8_t *col = (uint8_t*)&_col;
				for(; rpos.y < yend; rpos.y++) {
					uint8_t *addr = orgaddr;
					for(xcur = rpos.x; xcur < xend; xcur++) {
						*addr++ = *col;
						*addr++ = *(col + 1);
						*addr++ = *(col + 2);
					}
					orgaddr += widthadd;
				}
			}
			break;

			case 32: {
				for(; rpos.y < yend; rpos.y++) {
					uint32_t *addr = (uint32_t*)orgaddr;
					for(xcur = rpos.x; xcur < xend; xcur++)
						*addr++ = _col;
					orgaddr += widthadd;
				}
			}
			break;
		}
	}

	void Graphics::colorFadeRect(Orientation orientation,const Color &col1,const Color &col2,
		const Pos &pos,const Size &size) {
		if(!getPixels() || size.empty())
			return;

		int rdiff = col2.getRed() - col1.getRed();
		int gdiff = col2.getGreen() - col1.getGreen();
		int bdiff = col2.getBlue() - col1.getBlue();
		int adiff = col2.getAlpha() - col1.getAlpha();

		size_t colorfadelen = orientation == VERTICAL ? size.height : size.width;
		DblColor step(rdiff / (double)colorfadelen,gdiff / (double)colorfadelen,
			bdiff / (double)colorfadelen,adiff / (double)colorfadelen);
		DblColor cur(col1);

		Color old = getColor();
		setColor(col1);
		if(orientation == VERTICAL) {
			gpos_t endy = pos.y + size.height;
			for(gpos_t y = pos.y; y < endy; ++y) {
				drawHorLine(y,pos.x,pos.x + size.width);
				cur += step;
				setColor(cur.get());
			}
		}
		else {
			gpos_t endx = pos.x + size.width;
			for(gpos_t x = pos.x; x < endx; ++x) {
				drawVertLine(x,pos.y,pos.y + size.height);
				cur += step;
				setColor(cur.get());
			}
		}
		setColor(old);
	}

	void Graphics::fillTriangle(const Pos &p1,const Pos &p2,const Pos &p3) {
		if(!getPixels())
			return;

		// half-space function algorithm; based on devmaster.net/posts/6145/advanced-rasterization

		// 28.4 fixed-point coordinates
		const gpos_t y1 = p1.y << 4;
		const gpos_t y2 = p2.y << 4;
		const gpos_t y3 = p3.y << 4;

		const gpos_t x1 = p1.x << 4;
		const gpos_t x2 = p2.x << 4;
		const gpos_t x3 = p3.x << 4;

		// bounding rectangle
		gpos_t minx = (min(x1, min(x2, x3)) + 0xF) >> 4;
		gpos_t maxx = (max(x1, max(x2, x3)) + 0xF) >> 4;
		gpos_t miny = (min(y1, min(y2, y3)) + 0xF) >> 4;
		gpos_t maxy = (max(y1, max(y2, y3)) + 0xF) >> 4;
		bool res1 = validatePoint(minx,miny) != 0;
		bool res2 = validatePoint(maxx,maxy) != 0;
		if(res1 && res2)
			return;
		updateMinMax(Pos(minx,miny));
		updateMinMax(Pos(maxx,maxy));

		// deltas
		const gpos_t dx12 = x1 - x2;
		const gpos_t dx23 = x2 - x3;
		const gpos_t dx31 = x3 - x1;

		const gpos_t dy12 = y1 - y2;
		const gpos_t dy23 = y2 - y3;
		const gpos_t dy31 = y3 - y1;

		// fixed-point deltas
		const gpos_t fdx12 = dx12 << 4;
		const gpos_t fdx23 = dx23 << 4;
		const gpos_t fdx31 = dx31 << 4;

		const gpos_t fdy12 = dy12 << 4;
		const gpos_t fdy23 = dy23 << 4;
		const gpos_t fdy31 = dy31 << 4;

		// half-edge constants
		gpos_t c1 = dy12 * x1 - dx12 * y1;
		gpos_t c2 = dy23 * x2 - dx23 * y2;
		gpos_t c3 = dy31 * x3 - dx31 * y3;

		// correct for fill convention
		if(dy12 < 0 || (dy12 == 0 && dx12 > 0))
			c1++;
		if(dy23 < 0 || (dy23 == 0 && dx23 > 0))
			c2++;
		if(dy31 < 0 || (dy31 == 0 && dx31 > 0))
			c3++;

		gpos_t cy1 = c1 + dx12 * (miny << 4) - dy12 * (minx << 4);
		gpos_t cy2 = c2 + dx23 * (miny << 4) - dy23 * (minx << 4);
		gpos_t cy3 = c3 + dx31 * (miny << 4) - dy31 * (minx << 4);

		for(gpos_t y = miny; y < maxy; y++) {
			gpos_t cx1 = cy1;
			gpos_t cx2 = cy2;
			gpos_t cx3 = cy3;

			for(gpos_t x = minx; x < maxx; x++) {
				if(cx1 > 0 && cx2 > 0 && cx3 > 0)
					doSetPixel(x,y);

				cx1 -= fdy12;
				cx2 -= fdy23;
				cx3 -= fdy31;
			}

			cy1 += fdx12;
			cy2 += fdx23;
			cy3 += fdx31;
		}
	}

	void Graphics::drawCircle(const Pos &p,int radius) {
		// source: http://members.chello.at/~easyfilter/bresenham.html
		// TODO calling setPixel() that often isn't really fast
		gpos_t x = -radius;
		gpos_t y = 0;
		gpos_t err = 2 - 2 * radius;			/* II. Quadrant */
		do {
			setPixel(Pos(p.x - x,p.y + y));		/*   I. Quadrant */
			setPixel(Pos(p.x - y, p.y - x));	/*  II. Quadrant */
			setPixel(Pos(p.x + x, p.y - y));	/* III. Quadrant */
			setPixel(Pos(p.x + y, p.y + x));	/*  IV. Quadrant */

			radius = err;
			if(radius <= y)
				err += ++y * 2 + 1;				/* e_xy+e_y < 0 */
			if(radius > x || err > y)
				err += ++x * 2 + 1;				/* e_xy+e_x > 0 or no 2nd y-step */
		}
		while(x < 0);
	}

	void Graphics::fillCircle(const Pos &p,int radius) {
		gpos_t ystart = p.y - radius;
		gpos_t yend = p.y + radius;
		gpos_t xstart = p.x - radius;
		gpos_t xend = p.x + radius;
		bool res1 = validatePoint(xstart,ystart) != 0;
		bool res2 = validatePoint(xend,yend) != 0;
		if(res1 && res2)
			return;
		updateMinMax(Pos(xstart,ystart));
		updateMinMax(Pos(xend,yend));

		ystart -= p.y;
		yend -= p.y;
		xstart -= p.x;
		xend -= p.x;
		int r2 = radius * radius;
		for(gpos_t y = ystart; y <= yend; y++) {
			gpos_t y2 = y * y;
			for(gpos_t x = xstart; x <= xend; x++) {
				if((x * x) + y2 <= r2)
					doSetPixel(p.x + x,p.y + y);
			}
		}
	}

	void Graphics::requestUpdate() {
		Rectangle dirty = _buf->getDirtyRect();
		if(dirty.empty())
			return;
		if(!validateParams(dirty.getPos(),dirty.getSize()))
			return;

		_buf->requestUpdate(dirty.getPos(),dirty.getSize());
		_buf->resetDirty();
	}

	void Graphics::setSize(const Size &size) {
		_size.width = getDim(_off.x,size.width,_buf->getSize().width);
		_size.height = getDim(_off.y,size.height,_buf->getSize().height);
	}

	gsize_t Graphics::getDim(gpos_t off,gsize_t size,gsize_t max) {
		if((gpos_t)(off + size) > (gpos_t)max) {
			if(off < (gpos_t)max)
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
		gpos_t minx = _minoff.x - _off.x;
		gpos_t miny = _minoff.y - _off.y;
		if(x < minx) {
			x = minx;
			res |= OUT_LEFT;
		}
		if(y < miny) {
			y = miny;
			res |= OUT_TOP;
		}
		if(_off.x + x < 0) {
			x = -_off.x;
			res |= OUT_LEFT;
		}
		if(_off.y + y < 0) {
			y = -_off.y;
			res |= OUT_TOP;
		}
		if(x >= (gpos_t)_size.width + minx) {
			x = _size.width + minx - 1;
			res |= OUT_RIGHT;
		}
		if(y >= (gpos_t)_size.height + miny) {
			y = _size.height + miny - 1;
			res |= OUT_BOTTOM;
		}
		return res;
	}

	bool Graphics::validateParams(gpos_t &x,gpos_t &y,gsize_t &width,gsize_t &height) {
		if(_size.empty())
			return false;

		gpos_t minx = _minoff.x - _off.x;
		gpos_t miny = _minoff.y - _off.y;
		if(x < minx) {
			if((gpos_t)width < minx - x)
				return false;
			width -= minx - x;
			x = minx;
		}
		if(y < miny) {
			if((gpos_t)height < miny - y)
				return false;
			height -= miny - y;
			y = miny;
		}
		if(_off.y + y < 0) {
			if((gpos_t)height < -(_off.y + y))
				return false;
			height += _off.y + y;
			y = -_off.y;
		}
		if(_off.x + x < 0) {
			if((gpos_t)width < -(_off.x + x))
				return false;
			width += _off.x + x;
			x = -_off.x;
		}
		if(x + width > _size.width + minx)
			width = max(0,(gpos_t)_size.width + minx - x);
		if(y + height > _size.height + miny)
			height = max(0,(gpos_t)_size.height + miny - y);
		return width > 0 && height > 0;
	}
}
