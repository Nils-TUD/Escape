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

#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include <esc/common.h>
#include <esc/heap.h>
#include <esc/stream.h>
#include <esc/io.h>
#include <esc/messages.h>
#include <esc/mem.h>
#include <esc/gui/common.h>
#include <esc/gui/application.h>

// TODO ask vesa
#define RESOLUTION_X		1024
#define RESOLUTION_Y		768

namespace esc {
	namespace gui {

		// TODO make private
		typedef struct {
			sMsgHeader header;
			sMsgDataVesaUpdate data;
		} __attribute__((packed)) sMsgVesaUpdate;

		// TODO wrong place
		template<class T>
		void swap(T *a,T *b) {
			T t = *a;
			*a = *b;
			*b = t;
		}

		struct Pixel {
		public:
			Pixel() {};
			~Pixel() {};
			virtual u32 getPixelSize() const = 0;
			virtual tColor get(u32 offset) const = 0;
			virtual void set(u32 offset,tColor col) = 0;
		};

		struct Pixel16Bit : public Pixel {
		public:
			Pixel16Bit(u8 *pixels) : _pixels(pixels) {};
			~Pixel16Bit() {};

			inline u32 getPixelSize() const {
				return sizeof(u16);
			}
			inline tColor get(u32 offset) const {
				return *(u16*)(_pixels + offset * sizeof(u16));
			}
			inline void set(u32 offset,tColor col) {
				*(u16*)(_pixels + offset * sizeof(u16)) = col & 0xFFFF;
			};

		private:
			u8 *_pixels;
		};

		struct Pixel32Bit : public Pixel {
		public:
			Pixel32Bit(u8 *pixels) : _pixels(pixels) {};
			~Pixel32Bit() {};

			inline u32 getPixelSize() const {
				return sizeof(tColor);
			}
			inline tColor get(u32 offset) const {
				return *(tColor*)(_pixels + offset * sizeof(tColor));
			}
			inline void set(u32 offset,tColor col) {
				*(tColor*)(_pixels + offset * sizeof(tColor)) = col;
			};

		private:
			u8 *_pixels;
		};

		class Graphics {
			friend class Window;

		public:
			Graphics(tCoord x,tCoord y,tSize width,tSize height,tColDepth bpp) :
					_x(x), _y(y), _width(width), _height(height), _bpp(bpp), _col(0),
					_minx(0),_miny(0), _maxx(width - 1), _maxy(height - 1) {
				// allocate mem
				switch(_bpp) {
					case 32:
						_pixels = (u8*)calloc(_width * _height,sizeof(u32));
						_pixel = new Pixel32Bit(_pixels);
						break;
					case 16:
						_pixels = (u8*)calloc(_width * _height,sizeof(u16));
						_pixel = new Pixel16Bit(_pixels);
						break;
					default:
						err << "Unsupported color-depth: " << (u32)bpp << endl;
						break;
				}

				// set some constant msg-attributes
				_vesaMsg.header.id = MSG_VESA_UPDATE;
				_vesaMsg.header.length = sizeof(sMsgDataVesaUpdate);
			};
			~Graphics() {
				delete _pixel;
				delete _pixels;
			};

			inline tColor getColor() const {
				return _col;
			};
			inline void setColor(tColor col) {
				_col = col;
			};
			inline tColor getPixel(tCoord x,tCoord y) const {
				return _pixel->get(y * _width + x);
			};
			inline void setPixel(tCoord x,tCoord y) {
				x %= _width;
				y %= _height;
				updateMinMax(x,y);
				doSetPixel(x,y);
			};
			void drawLine(tCoord x0,tCoord y0,tCoord xn,tCoord yn);
			void drawRect(tCoord x,tCoord y,tSize width,tSize height);
			void fillRect(tCoord x,tCoord y,tSize width,tSize height);
			void update();
			void update(tCoord x,tCoord y,tSize width,tSize height);
			void debug() const;

		private:
			inline void doSetPixel(tCoord x,tCoord y) {
				_pixel->set(y * _width + x,_col);
			};
			inline void updateMinMax(tCoord x,tCoord y) {
				if(x > _maxx)
					_maxx = x;
				else if(x < _minx)
					_minx = x;
				if(y > _maxy)
					_maxy = y;
				else if(y < _miny)
					_miny = y;
			};
			void move(tCoord x,tCoord y);
			void clear();
			void notifyVesa(tCoord x,tCoord y,tSize width,tSize height);
			void validateParams(tCoord &x,tCoord &y,tSize &width,tSize &height);

		private:
			tCoord _x,_y;
			tSize _width;
			tSize _height;
			tColDepth _bpp;
			tColor _col;
			tCoord _minx,_miny,_maxx,_maxy;
			Pixel *_pixel;
			u8 *_pixels;
			sMsgVesaUpdate _vesaMsg;
		};
	}
}

#endif /* GRAPHICS_H_ */
