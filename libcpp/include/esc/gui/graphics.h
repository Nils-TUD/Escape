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
			Graphics(Graphics &g,tCoord x,tCoord y);
			Graphics(tCoord x,tCoord y,tSize width,tSize height,tColDepth bpp);
			~Graphics();

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
			void moveLines(tCoord y,tSize height,tSize up);
			void drawChar(tCoord x,tCoord y,char c);
			void drawString(tCoord x,tCoord y,const String &str);
			void drawLine(tCoord x0,tCoord y0,tCoord xn,tCoord yn);
			void drawRect(tCoord x,tCoord y,tSize width,tSize height);
			void fillRect(tCoord x,tCoord y,tSize width,tSize height);
			void requestUpdate(tWinId winid);
			void update(tCoord x,tCoord y,tSize width,tSize height);
			void debug() const;

		private:
			// prevent the compiler from generating copy-constructor and assignment-operator
			// by declaring them with out definition
			Graphics(const Graphics &g);
			Graphics &operator=(const Graphics &g);

			void doSetPixel(tCoord x,tCoord y) {
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
			};
			inline void updateMinMax(tCoord x,tCoord y) {
				if(x > _maxx)
					_maxx = x;
				if(x < _minx)
					_minx = x;
				if(y > _maxy)
					_maxy = y;
				if(y < _miny)
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
