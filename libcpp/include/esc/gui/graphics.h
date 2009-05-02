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
#include <esc/proc.h>
#include <esc/messages.h>
#include <esc/mem.h>
#include <esc/gui/common.h>
#include <esc/gui/application.h>
#include <esc/gui/font.h>
#include <esc/gui/color.h>
#include <esc/string.h>
#include <stdlib.h>

namespace esc {
	namespace gui {
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
			virtual u32 get(u32 offset) const = 0;
			virtual void set(u32 offset,u32 col) = 0;
		};

		struct Pixel8Bit : public Pixel {
		public:
			Pixel8Bit(u8 *pixels) : _pixels(pixels) {};
			~Pixel8Bit() {};

			inline u32 getPixelSize() const {
				return sizeof(u8);
			}
			inline u32 get(u32 offset) const {
				return *(u8*)(_pixels + offset * sizeof(u8));
			}
			inline void set(u32 offset,u32 col) {
				*(u8*)(_pixels + offset * sizeof(u8)) = col & 0xFF;
			};

		private:
			u8 *_pixels;
		};

		struct Pixel16Bit : public Pixel {
		public:
			Pixel16Bit(u8 *pixels) : _pixels(pixels) {};
			~Pixel16Bit() {};

			inline u32 getPixelSize() const {
				return sizeof(u16);
			}
			inline u32 get(u32 offset) const {
				return *(u16*)(_pixels + offset * sizeof(u16));
			}
			inline void set(u32 offset,u32 col) {
				*(u16*)(_pixels + offset * sizeof(u16)) = col & 0xFFFF;
			};

		private:
			u8 *_pixels;
		};

		struct Pixel24Bit : public Pixel {
		public:
			Pixel24Bit(u8 *pixels) : _pixels(pixels) {};
			~Pixel24Bit() {};

			inline u32 getPixelSize() const {
				return 3;
			}
			inline u32 get(u32 offset) const {
				u32 col;
				memcpy((u8*)&col + 1,_pixels + offset * 3,3);
				return col;
			}
			inline void set(u32 offset,u32 col) {
				memcpy(_pixels + offset * 3,&col,3);
			};

		private:
			u8 *_pixels;
		};

		struct Pixel32Bit : public Pixel {
		public:
			Pixel32Bit(u8 *pixels) : _pixels(pixels) {};
			~Pixel32Bit() {};

			inline u32 getPixelSize() const {
				return sizeof(u32);
			}
			inline u32 get(u32 offset) const {
				return *(u32*)(_pixels + offset * sizeof(u32));
			}
			inline void set(u32 offset,u32 col) {
				*(u32*)(_pixels + offset * sizeof(u32)) = col;
			};

		private:
			u8 *_pixels;
		};

		class Graphics {
			friend class Window;
			friend class Control;

		private:
			typedef struct {
				sMsgHeader header;
				sMsgDataVesaUpdate data;
			} __attribute__((packed)) sMsgVesaUpdate;

		public:
			Graphics(Graphics &g,tCoord x,tCoord y)
				: _offx(x), _offy(y), _x(0), _y(0), _width(g._width), _height(g._height),
					_bpp(g._bpp), _col(0), _minx(0),_miny(0), _maxx(_width - 1),
					_maxy(_height - 1), _font(Font()), _owner(&g) {
				_pixels = g._pixels;
				_pixel = g._pixel;
				// set some constant msg-attributes
				_vesaMsg.header.id = MSG_VESA_UPDATE;
				_vesaMsg.header.length = sizeof(sMsgDataVesaUpdate);
			};

			Graphics(tCoord x,tCoord y,tSize width,tSize height,tColDepth bpp) :
					_offx(0), _offy(0), _x(x), _y(y), _width(width), _height(height), _bpp(bpp),
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

				// set some constant msg-attributes
				_vesaMsg.header.id = MSG_VESA_UPDATE;
				_vesaMsg.header.length = sizeof(sMsgDataVesaUpdate);
			};

			~Graphics() {
				if(_owner == NULL) {
					delete _pixel;
					delete _pixels;
				}
			};

			inline Font getFont() const {
				return _font;
			}
			inline Color getColor() const {
				return Color::fromBits(_col,_bpp);
			};
			inline void setColor(const Color &col) {
				_col = col.toBits(_bpp);
			};
			inline Color getPixel(tCoord x,tCoord y) const {
				return Color::fromBits(_pixel->get(y * _width + x),_bpp);
			};
			inline void setPixel(tCoord x,tCoord y) {
				x %= _width;
				y %= _height;
				updateMinMax(x,y);
				doSetPixel(x,y);
			};
			inline tColDepth getColorDepth() const {
				return _bpp;
			};
			void drawChar(tCoord x,tCoord y,char c);
			void drawString(tCoord x,tCoord y,String str);
			void drawLine(tCoord x0,tCoord y0,tCoord xn,tCoord yn);
			void drawRect(tCoord x,tCoord y,tSize width,tSize height);
			void fillRect(tCoord x,tCoord y,tSize width,tSize height);
			void update();
			void update(tCoord x,tCoord y,tSize width,tSize height);
			void debug() const;

		private:
			inline void doSetPixel(tCoord x,tCoord y) {
				_pixel->set((_offy + y) * _width + (_offx + x),_col);
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
			void notifyVesa(tCoord x,tCoord y,tSize width,tSize height);
			void validateParams(tCoord &x,tCoord &y,tSize &width,tSize &height);

		private:
			tCoord _offx,_offy;
			tCoord _x,_y;
			tSize _width;
			tSize _height;
			tColDepth _bpp;
			u32 _col;
			tCoord _minx,_miny,_maxx,_maxy;
			Pixel *_pixel;
			u8 *_pixels;
			Font _font;
			sMsgVesaUpdate _vesaMsg;
			Graphics *_owner;
		};
	}
}

#endif /* GRAPHICS_H_ */
