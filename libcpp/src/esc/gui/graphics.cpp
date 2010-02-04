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
#include <esc/gui/graphics.h>
#include <esc/gui/window.h>
#include <esc/string.h>
#include <string.h>

namespace esc {
	namespace gui {
		Graphics::Graphics(Graphics &g,tCoord x,tCoord y)
			: _offx(x), _offy(y), _x(0), _y(0), _width(g._width), _height(g._height),
				_bpp(g._bpp), _col(0), _minx(0),_miny(0), _maxx(_width - 1),
				_maxy(_height - 1), _font(Font()), _owner(&g) {
			_pixels = g._pixels;
			_pixel = g._pixel;
		}

		Graphics::Graphics(tCoord x,tCoord y,tSize width,tSize height,tColDepth bpp)
			: _offx(0), _offy(0), _x(x), _y(y), _width(width), _height(height), _bpp(bpp),
				_col(0), _minx(0),_miny(0), _maxx(width - 1), _maxy(height - 1),_font(Font()),
				_owner(NULL) {
			// allocate mem
			switch(_bpp) {
				case 32:
					_pixels = (u8*)calloc(_width * _height,sizeof(u32));
					_pixel = new Pixel32Bit(_pixels);
					break;
				case 24:
					_pixels = (u8*)calloc(_width * _height,3);
					_pixel = new Pixel24Bit(_pixels);
					break;
				case 16:
					_pixels = (u8*)calloc(_width * _height,sizeof(u16));
					_pixel = new Pixel16Bit(_pixels);
					break;
				case 8:
					_pixels = (u8*)calloc(_width * _height,sizeof(u8));
					_pixel = new Pixel8Bit(_pixels);
					break;
				default:
					err << "Unsupported color-depth: " << (u32)bpp << endl;
					exit(EXIT_FAILURE);
					break;
			}
		}

		Graphics::~Graphics() {
			if(_owner == NULL) {
				delete _pixel;
				delete _pixels;
			}
		}

		void Graphics::doSetPixel(tCoord x,tCoord y) {
			u32 offset = (_offy + y) * _width + (_offx + x);
			switch(_bpp) {
				case 8:
					*(u8*)(_pixels + offset * sizeof(u8)) = _col & 0xFF;
					break;
				case 16:
					*(u16*)(_pixels + offset * sizeof(u16)) = _col & 0xFFFF;
					break;
				case 24: {
					u8 *col = (u8*)&_col;
					u8 *addr = _pixels + offset * 3;
					*addr++ = *col++;
					*addr++ = *col++;
					*addr++ = *col++;
				}
				break;
				case 32:
					*(u32*)(_pixels + offset * sizeof(u32)) = _col;
					break;
			}
			//_pixel->set((_offy + y) * _width + (_offx + x),_col);
		}

		void Graphics::moveLines(tCoord y,tSize height,s16 up) {
			tCoord x = 0;
			tSize width = _width;
			validateParams(x,y,width,height);
			tCoord starty = _offy + y;
			u32 psize = _pixel->getPixelSize();
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

		void Graphics::copyLine(tCoord x,tCoord y,tSize width,void *line) {
			u32 psize = _pixel->getPixelSize();
			y %= _height;
			width = MIN(width,_width);
			memcpy(_pixels + ((_offy + y) * _width + (_offx + x)) * psize,line,width * psize);
			updateMinMax(x,y);
			updateMinMax(x + width,y);
		}

		void Graphics::drawChar(tCoord x,tCoord y,char c) {
			char *font = _font.getChar(c);
			if(font) {
				tSize width = _font.getWidth();
				tSize height = _font.getHeight();
				validateParams(x,y,width,height);
				updateMinMax(x,y);
				updateMinMax(x + width - 1,y + height - 1);
				tCoord cx,cy;
				for(cy = 0; cy < height; cy++) {
					for(cx = 0; cx < width; cx++) {
						if(*font)
							doSetPixel(x + cx,y + cy);
						font++;
					}
				}
			}
		}

		void Graphics::drawString(tCoord x,tCoord y,const String &str) {
			u32 charWidth = _font.getWidth();
			for(u32 i = 0; i < str.length(); i++) {
				drawChar(x,y,str[i]);
				x += charWidth;
			}
		}

		void Graphics::drawLine(tCoord x0,tCoord y0,tCoord xn,tCoord yn) {
			s32	dx,	dy,	d;
			s32 incrE, incrNE;	/*Increments for move to E	& NE*/
			tCoord x,y;			/*Start & current pixel*/
			s32 incx, incy;
			tCoord *px, *py;

			x0 %= _width;
			xn %= _width;
			y0 %= _height;
			yn %= _height;

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
				swap<tCoord>(&x,&y);
				swap<tCoord*>(&px,&py);
				swap<s32>(&dx,&dy);
				swap<tCoord>(&x0,&y0);
				swap<tCoord>(&xn,&yn);
				swap<s32>(&incx,&incy);
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

		void Graphics::drawRect(tCoord x,tCoord y,tSize width,tSize height) {
			// top
			drawLine(x,y,x + width - 1,y);
			// right
			drawLine(x + width - 1,y,x + width - 1,y + height - 1);
			// bottom
			drawLine(x + width - 1,y + height - 1,x,y + height - 1);
			// left
			drawLine(x,y + height - 1,x,y);
		}

		void Graphics::fillRect(tCoord x,tCoord y,tSize width,tSize height) {
			validateParams(x,y,width,height);
			tCoord yend = y + height;
			updateMinMax(x,y);
			updateMinMax(x + width - 1,yend - 1);
			tCoord xcur;
			tCoord xend = x + width;
			if(_pixel->getPixelSize() == 3) {
				// optimized version for 24bit
				// This is necessary if we want to have reasonable speed because the simple version
				// performs too many function-calls (one to a virtual-function and one to memcpy
				// that the compiler doesn't inline). Additionally the offset into the
				// memory-region will be calculated many times.
				// This version is much quicker :)
				u8 *col = (u8*)&_col;
				u32 widthadd = _width * 3;
				u8 *addr;
				u8 *orgaddr = _pixels + (((_offy + y) * _width + (_offx + x)) * 3);
				for(; y < yend; y++) {
					addr = orgaddr;
					for(xcur = x; xcur < xend; xcur++) {
						*addr++ = *col;
						*addr++ = *(col + 1);
						*addr++ = *(col + 2);
					}
					orgaddr += widthadd;
				}
			}
			else {
				// TODO write optimized version for 8,16 and 32 bit
				for(; y < yend; y++) {
					for(xcur = x; xcur < xend; xcur++)
						doSetPixel(xcur,y);
				}
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
				width = MIN(screenWidth - x - _x,MIN(_width - x,width));
				height = MIN(screenHeight - y - _y,MIN(_height - y,height));
				void *vesaMem = Application::getInstance()->getVesaMem();
				u8 *src,*dst;
				tCoord endy = y + height;
				u32 psize = _pixel->getPixelSize();
				u32 count = width * psize;
				u32 srcAdd = _width * psize;
				u32 dstAdd = screenWidth * psize;
				src = _pixels + (y * _width + x) * psize;
				dst = (u8*)vesaMem + ((_y + y) * screenWidth + (_x + x)) * psize;
				while(y < endy) {
					memcpy(dst,src,count);
					src += srcAdd;
					dst += dstAdd;
					y++;
				}

				notifyVesa(_x + x,_y + endy - height,width,height);
				/*yield();*/
			}
		}

		void Graphics::notifyVesa(tCoord x,tCoord y,tSize width,tSize height) {
			tFD vesaFd = Application::getInstance()->getVesaFd();
			sMsg msg;
			msg.args.arg1 = x;
			msg.args.arg2 = y;
			msg.args.arg3 = width;
			msg.args.arg4 = height;
			send(vesaFd,MSG_VESA_UPDATE,&msg,sizeof(msg.args));
		}

		void Graphics::move(tCoord x,tCoord y) {
			tSize screenWidth = Application::getInstance()->getScreenWidth();
			tSize screenHeight = Application::getInstance()->getScreenHeight();
			_x = MIN(screenWidth - 1,x);
			_y = MIN(screenHeight - 1,y);
		}

		void Graphics::debug() const {
			for(tCoord y = 0; y < _height; y++) {
				for(tCoord x = 0; x < _width; x++)
					out << (getPixel(x,y).getColor() == 0 ? ' ' : 'x');
				out << endl;
			}
		}

		void Graphics::validateParams(tCoord &x,tCoord &y,tSize &width,tSize &height) {
			x %= _width;
			y %= _height;
			if(_offx + x + width > _width)
				width = MAX(0,(s32)_width - x - _offx);
			if(_offy + y + height > _height)
				height = MAX(0,(s32)_height - y - _offy);
		}
	}
}
