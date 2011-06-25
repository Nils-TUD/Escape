/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
	Graphics::Graphics(Graphics &g,tCoord x,tCoord y)
		: _offx(x), _offy(y), _x(0), _y(0), _width(g._width), _height(g._height),
			_bpp(g._bpp), _col(0), _colInst(Color(0)), _minx(0),_miny(0), _maxx(_width - 1),
			_maxy(_height - 1), _pixels(NULL), _font(Font()), _owner(&g) {
		_pixels = g._pixels;
	}

	Graphics::Graphics(tCoord x,tCoord y,tSize width,tSize height,tColDepth bpp)
		: _offx(0), _offy(0), _x(x), _y(y), _width(width), _height(height), _bpp(bpp),
			_col(0), _colInst(Color(0)), _minx(0),_miny(0), _maxx(width - 1), _maxy(height - 1),
			_pixels(NULL), _font(Font()), _owner(NULL) {
		allocBuffer();
	}

	Graphics::~Graphics() {
		if(_owner == NULL)
			delete _pixels;
	}

	void Graphics::allocBuffer() {
		switch(_bpp) {
			case 32:
				_pixels = (uint8_t*)calloc(_width * _height,4);
				break;
			case 24:
				_pixels = (uint8_t*)calloc(_width * _height,3);
				break;
			case 16:
				_pixels = (uint8_t*)calloc(_width * _height,2);
				break;
			default:
				cerr << "Unsupported color-depth: " << _bpp << endl;
				exit(EXIT_FAILURE);
				break;
		}
		if(!_pixels) {
			cerr << "Not enough memory" << endl;
			exit(EXIT_FAILURE);
		}
	}

	void Graphics::moveLines(tCoord y,tSize height,int up) {
		tCoord x = 0;
		tSize width = _width;
		validateParams(x,y,width,height);
		tCoord starty = _offy + y;
		tSize psize = _bpp / 8;
		if(up > 0) {
			if(y < up)
				up = y;
		}
		else {
			if(y + height - up > _height)
				up = -(_height - (height + y));
		}
		memmove(_pixels + ((starty - up) * _width) * psize,
				_pixels + (starty * _width) * psize,
				height * _width * psize);
		updateMinMax(0,y - up);
		updateMinMax(_width - 1,y + height - up - 1);
	}

	void Graphics::drawChar(tCoord x,tCoord y,char c) {
		tSize width = _font.getWidth();
		tSize height = _font.getHeight();
		validateParams(x,y,width,height);
		updateMinMax(x,y);
		updateMinMax(x + width - 1,y + height - 1);
		tCoord cx,cy;
		for(cy = 0; cy < height; cy++) {
			for(cx = 0; cx < width; cx++) {
				if(_font.isPixelSet(c,cx,cy))
					doSetPixel(x + cx,y + cy);
			}
		}
	}

	void Graphics::drawString(tCoord x,tCoord y,const string &str) {
		drawString(x,y,str,0,str.length());
	}

	void Graphics::drawString(tCoord x,tCoord y,const string &str,size_t start,size_t count) {
		size_t charWidth = _font.getWidth();
		size_t end = start + MIN(str.length(),count);
		for(size_t i = start; i < end; i++) {
			drawChar(x,y,str[i]);
			x += charWidth;
		}
	}

	void Graphics::drawLine(tCoord x0,tCoord y0,tCoord xn,tCoord yn) {
		int	dx,	dy,	d;
		int incrE, incrNE;	/*Increments for move to E	& NE*/
		tCoord x,y;			/*Start & current pixel*/
		int incx, incy;
		tCoord *px, *py;

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
			if(d < 0) {
				d += incrE;
			}
			else {
				d += incrNE;
				y += incy;
			}
		}
	}

	void Graphics::drawVertLine(tCoord x,tCoord y1,tCoord y2) {
		validatePos(x,y1);
		validatePos(x,y2);
		updateMinMax(x,y1);
		updateMinMax(x,y2);
		if(y1 > y2)
			std::swap(y1,y2);
		for(; y1 < y2; y1++)
			doSetPixel(x,y1);
	}

	void Graphics::drawHorLine(tCoord y,tCoord x1,tCoord x2) {
		validatePos(x1,y);
		validatePos(x2,y);
		updateMinMax(x1,y);
		updateMinMax(x2,y);
		if(x1 > x2)
			std::swap(x1,x2);
		for(; x1 < x2; x1++)
			doSetPixel(x1,y);
	}

	void Graphics::drawRect(tCoord x,tCoord y,tSize width,tSize height) {
		// top
		drawHorLine(y,x,x + width - 1);
		// right
		drawVertLine(x + width - 1,y,y + height - 1);
		// bottom
		drawHorLine(y + height - 1,x,x + width - 1);
		// left
		drawVertLine(x,y,y + height - 1);
	}

	void Graphics::fillRect(tCoord x,tCoord y,tSize width,tSize height) {
		validateParams(x,y,width,height);
		tCoord yend = y + height;
		updateMinMax(x,y);
		updateMinMax(x + width - 1,yend - 1);
		tCoord xcur;
		tCoord xend = x + width;
		for(; y < yend; y++) {
			for(xcur = x; xcur < xend; xcur++)
				doSetPixel(xcur,y);
		}
	}

	void Graphics::requestUpdate(tWinId winid) {
		Window *w = Application::getInstance()->getWindowById(winid);
		if(w->isCreated()) {
			// if we are the active (=top) window, we can update directly
			if(w->isActive()) {
				if(_owner != NULL)
					_owner->update(_offx + _minx,_offy + _miny,_maxx - _minx + 1,_maxy - _miny + 1);
				else
					update(_offx + _minx,_offy + _miny,_maxx - _minx + 1,_maxy - _miny + 1);
			}
			else {
				// notify winmanager that we want to repaint this area; after a while we'll get multiple
				// (ok, maybe just one) update-events with specific areas to update
				Application::getInstance()->requestWinUpdate(winid,_offx + _minx,_offy + _miny,
						_maxx - _minx + 1,_maxy - _miny + 1);
			}
		}

		// reset region
		_minx = _width - 1;
		_maxx = 0;
		_miny = _height - 1;
		_maxy = 0;
	}

	void Graphics::update(tCoord x,tCoord y,tSize width,tSize height) {
		// only the owner notifies vesa
		if(_owner != NULL) {
			_owner->update(_owner->_x + x,_owner->_y + y,width,height);
			return;
		}

		validateParams(x,y,width,height);
		// is there anything to update?
		if(width > 0 || height > 0) {
			tSize screenWidth = Application::getInstance()->getScreenWidth();
			tSize screenHeight = Application::getInstance()->getScreenHeight();
			if(_x + x >= screenWidth || _y + y >= screenHeight)
				return;
			if(_x < 0 && _x + x < 0) {
				width += _x + x;
				x = -_x;
			}
			width = MIN(screenWidth - (x + _x),MIN(_width - x,width));
			height = MIN(screenHeight - (y + _y),MIN(_height - y,height));
			void *vesaMem = Application::getInstance()->getVesaMem();
			uint8_t *src,*dst;
			tCoord endy = y + height;
			size_t psize = _bpp / 8;
			size_t count = width * psize;
			size_t srcAdd = _width * psize;
			size_t dstAdd = screenWidth * psize;
			src = _pixels + (y * _width + x) * psize;
			dst = (uint8_t*)vesaMem + ((_y + y) * screenWidth + (_x + x)) * psize;
			while(y < endy) {
				memcpy(dst,src,count);
				src += srcAdd;
				dst += dstAdd;
				y++;
			}

			notifyVesa(_x + x,_y + endy - height,width,height);
		}
	}

	void Graphics::notifyVesa(tCoord x,tCoord y,tSize width,tSize height) {
		int vesaFd = Application::getInstance()->getVesaFd();
		sMsg msg;
		if(x < 0) {
			width += x;
			x = 0;
		}
		msg.args.arg1 = x;
		msg.args.arg2 = y;
		msg.args.arg3 = width;
		msg.args.arg4 = height;
		if(send(vesaFd,MSG_VESA_UPDATE,&msg,sizeof(msg.args)) < 0)
			cerr << "Unable to send update-request to VESA" << endl;
	}

	void Graphics::moveTo(tCoord x,tCoord y) {
		tSize screenWidth = Application::getInstance()->getScreenWidth();
		tSize screenHeight = Application::getInstance()->getScreenHeight();
		_x = MIN(screenWidth - 1,x);
		_y = MIN(screenHeight - 1,y);
	}

	void Graphics::resizeTo(tSize width,tSize height) {
		_width = width;
		_height = height;
		if(_owner == NULL) {
			delete [] _pixels;
			allocBuffer();
		}
		else
			_pixels = _owner->_pixels;
	}

	void Graphics::validatePos(tCoord &x,tCoord &y) {
		if(x >= _width - _offx)
			x = _width - _offx - 1;
		if(y >= _height - _offy)
			y = _height - _offy - 1;
	}

	void Graphics::validateParams(tCoord &x,tCoord &y,tSize &width,tSize &height) {
		if(_offx + x + width > _width)
			width = MAX(0,(int)_width - x - _offx);
		if(_offy + y + height > _height)
			height = MAX(0,(int)_height - y - _offy);
	}
}
