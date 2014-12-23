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

#pragma once

#include <sys/common.h>
#include <gui/graphics/graphicsbuffer.h>
#include <gui/graphics/font.h>
#include <gui/graphics/color.h>
#include <gui/graphics/size.h>
#include <gui/graphics/pos.h>
#include <gui/graphics/rectangle.h>
#include <gui/application.h>
#include <gui/enums.h>
#include <math.h>
#include <assert.h>

namespace gui {
	/**
	 * The graphics-class is responsible for drawing on a buffer, copying it to the shared-
	 * memory-region offered by vesa and notifying vesa for updates. It offers several
	 * drawing routines like drawLine, drawRect, fillRect and so on.
	 */
	class Graphics {
		friend class Window;
		friend class Control;
		friend class UIElement;
		friend class Image;
		friend class GUIPainter;

	private:
		static const int OUT_TOP		= 1;
		static const int OUT_RIGHT		= 2;
		static const int OUT_BOTTOM		= 4;
		static const int OUT_LEFT		= 8;

		struct DblColor {
			explicit DblColor(const Color &c) : r(c.getRed()), g(c.getGreen()), b(c.getBlue()), a(c.getAlpha()) {
			}
			explicit DblColor(double _r,double _g,double _b,double _a) : r(_r), g(_g), b(_b), a(_a) {
			}

			Color get() const {
				Color::comp_type rc = std::max(0,std::min(255,static_cast<int>(round(r))));
				Color::comp_type gc = std::max(0,std::min(255,static_cast<int>(round(g))));
				Color::comp_type bc = std::max(0,std::min(255,static_cast<int>(round(b))));
				Color::comp_type ac = std::max(0,std::min(255,static_cast<int>(round(a))));
				return Color(rc,gc,bc,ac);
			}

			void operator+=(const DblColor &c) {
				r += c.r;
				g += c.g;
				b += c.b;
				a += c.a;
			}

			double r,g,b,a;
		};

	public:
		/**
		 * Constructor
		 *
		 * @param buf the graphics-buffer for the window
		 * @param size the size of the control
		 */
		Graphics(GraphicsBuffer *buf,const Size &size)
			: _buf(buf), _minoff(), _off(), _size(size), _col(0), _colInst(0), _font() {
		}

		/**
		 * @return the current font
		 */
		const Font &getFont() const {
			return _font;
		}

		/**
		 * @return the current color
		 */
		Color getColor() const {
			return _colInst;
		}

		/**
		 * Sets the color
		 *
		 * @param col the new color
		 */
		void setColor(const Color &col) {
			_col = col.toCurMode();
			_colInst = col;
		}

		/**
		 * Sets the pixel to the current color
		 *
		 * @param x the x-coordinate
		 * @param y the y-coordinate
		 */
		void setPixel(const Pos &pos) {
			Pos rpos = pos;
			if(!getPixels() || validatePoint(rpos.x,rpos.y) != 0)
				return;
			updateMinMax(rpos);
			doSetPixel(rpos.x,rpos.y);
		}

		/**
		 * Moves <height> rows of <width> pixels at <pos> up by <up> rows.
		 * If <up> is negative, it moves them down.
		 *
		 * @param pos the coordinates where to start
		 * @param size the width of a row and the number of rows to move
		 * @param up amount to move up / down
		 */
		void moveRows(const Pos &pos,const Size &size,int up);
		void moveRows(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int up) {
			moveRows(Pos(x,y),Size(width,height),up);
		}

		/**
		 * Moves <width> cols of <height> pixels at <pos> left by <left> cols.
		 * If <left> is negative, it moves them right.
		 *
		 * @param pos the coordinates where to start
		 * @param size the number of cols to move and the height of a col
		 * @param left amount to move left / right
		 */
		void moveCols(const Pos &pos,const Size &size,int left);
		void moveCols(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int left) {
			moveCols(Pos(x,y),Size(width,height),left);
		}

		/**
		 * Draws the given character at given position
		 *
		 * @param pos the position
		 * @param c the character
		 */
		void drawChar(const Pos &pos,char c);
		void drawChar(gpos_t x,gpos_t y,char c) {
			drawChar(Pos(x,y),c);
		}

		/**
		 * Draws the given string at the given position. Note that the position is the top
		 * left of the first character.
		 *
		 * @param pos the position
		 * @param str the string
		 */
		void drawString(const Pos &pos,const std::string &str);
		void drawString(gpos_t x,gpos_t y,const std::string &str) {
			drawString(Pos(x,y),str);
		}

		/**
		 * Draws the given part of the given string at the given position. Note that the
		 * position is the top left of the first character.
		 *
		 * @param pos the position
		 * @param str the string
		 * @param start the start-position in the string
		 * @param count the number of characters
		 */
		void drawString(const Pos &pos,const std::string &str,size_t start,size_t count);
		void drawString(gpos_t x,gpos_t y,const std::string &str,size_t start,size_t count) {
			drawString(Pos(x,y),str,start,count);
		}

		/**
		 * Draws a line from p0 to pn
		 *
		 * @param p0 first coordinate
		 * @param pn last coordinate
		 */
		void drawLine(const Pos &p0,const Pos &pn);
		void drawLine(gpos_t x0,gpos_t y0,gpos_t xn,gpos_t yn) {
			drawLine(Pos(x0,y0),Pos(xn,yn));
		}

		/**
		 * Draws a vertical line (optimized compared to drawLine)
		 *
		 * @param x the x-coordinate
		 * @param y1 the first y-coordinate
		 * @param y2 the second y-coordinate
		 */
		void drawVertLine(gpos_t x,gpos_t y1,gpos_t y2);

		/**
		 * Draws a horizontal line (optimized compared to drawLine)
		 *
		 * @param y the y-coordinate
		 * @param x1 the first x-coordinate
		 * @param x2 the second x-coordinate
		 */
		void drawHorLine(gpos_t y,gpos_t x1,gpos_t x2);

		/**
		 * Draws a rectangle
		 *
		 * @param pos the position
		 * @param size the size
		 */
		void drawRect(const Pos &pos,const Size &size);
		void drawRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
			drawRect(Pos(x,y),Size(width,height));
		}

		/**
		 * Fills a rectangle
		 *
		 * @param pos the position
		 * @param size the size
		 */
		void fillRect(const Pos &pos,const Size &size);
		void fillRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
			fillRect(Pos(x,y),Size(width,height));
		}

		/**
		 * Fills the given rectangle with a color-fade from <col1> to <col2>.
		 *
		 * @param orientation whether the color-fade should be horizontal or vertical
		 * @param col1 the start-color
		 * @param col2 the end-color
		 * @param pos the position
		 * @param size the size
		 */
		void colorFadeRect(Orientation orientation,const Color &col1,const Color &col2,
			const Pos &pos,const Size &size);
		void colorFadeRect(Orientation orientation,const Color &col1,const Color &col2,
			gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
			colorFadeRect(orientation,col1,col2,Pos(x,y),Size(width,height));
		}

		/**
		 * Draws the triangle defined by <p1>, <p2> and <p3>.
		 *
		 * @param p1 the first point
		 * @param p2 the second point
		 * @param p3 the third point
		 */
		void drawTriangle(const Pos &p1,const Pos &p2,const Pos &p3) {
			drawLine(p1,p2);
			drawLine(p2,p3);
			drawLine(p3,p1);
		}

		/**
		 * Fills the triangle defined by <p1>, <p2> and <p3>. Note that the three points HAVE TO be
		 * in counter-clockwise order!
		 *
		 * @param p1 the first point
		 * @param p2 the second point
		 * @param p3 the third point
		 */
		void fillTriangle(const Pos &p1,const Pos &p2,const Pos &p3);

		/**
		 * Draws a circle with the center at position <p> and given radius.
		 *
		 * @param p the center of the circle
		 * @param radius the radius
		 */
		void drawCircle(const Pos &p,int radius);

		/**
		 * Fills a circle with the center at position <p> and given radius.
		 *
		 * @param p the center of the circle
		 * @param radius the radius
		 */
		void fillCircle(const Pos &p,int radius);

		/**
		 * @return the offset of the control in the window
		 */
		Pos getPos() const {
			return _off;
		}

		/**
		 * @return the size of the paintable area
		 */
		Size getSize() const {
			return _size;
		}

		/**
		 * @return the current paint rectangle. every paint outside this rectangle is ignored.
		 */
		Rectangle getPaintRect() {
			return Rectangle(_minoff - _off, _size);
		}

	protected:
		/**
		 * @return the pixel buffer (might be nullptr)
		 */
		uint8_t *getPixels() {
			// don't paint anything if a transparent color is set
			return _colInst.isTransparent() ? nullptr : _buf->getBuffer();
		}
		/**
		 * Sets a pixel if in the given bounds
		 */
		void setLinePixel(gpos_t minx,gpos_t miny,gpos_t maxx,gpos_t maxy,const Pos &pos) {
			if(pos.x >= minx && pos.y >= miny && pos.x <= maxx && pos.y <= maxy)
				doSetPixel(pos.x,pos.y);
		}
		/**
		 * Sets a pixel (without check)
		 */
		void doSetPixel(gpos_t x,gpos_t y) {
			gcoldepth_t bpp = Application::getInstance()->getColorDepth();
			size_t bytespp = bpp / 8;
			gsize_t bwidth = _buf->getSize().width;
			uint8_t *addr = getPixels() + ((_off.y + y) * bwidth + (_off.x + x)) * bytespp;

			switch(bpp) {
				case 16:
					*(uint16_t*)addr = _col;
					break;
				case 24: {
					uint8_t *col = (uint8_t*)&_col;
					*addr++ = *col++;
					*addr++ = *col++;
					*addr = *col;
				}
				break;
				case 32:
					*(uint32_t*)addr = _col;
					break;
			}
		}
		/**
		 * Adds the given position to the dirty region
		 */
		void updateMinMax(const Pos &pos) {
			_buf->updateDirty(Pos(_off.x,_off.y) + pos);
		}
		/**
		 * Requests an update for the dirty region
		 */
		void requestUpdate();
		/**
		 * Validates the given line
		 */
		bool validateLine(gpos_t &x1,gpos_t &y1,gpos_t &x2,gpos_t &y2);
		/**
		 * Validates the given point
		 */
		int validatePoint(gpos_t &x,gpos_t &y);
		/**
		 * Validates the given parameters
		 */
		bool validateParams(gpos_t &x,gpos_t &y,gsize_t &width,gsize_t &height);
		bool validateParams(Pos &pos,Size &size) {
			return validateParams(pos.x,pos.y,size.width,size.height);
		}

	private:
		/**
		 * @return the graphics-buffer
		 */
		GraphicsBuffer* getBuffer() const {
			return _buf;
		}
		/**
		 * Sets the offset of the control in the window
		 *
		 * @param x the x-offset
		 * @param y the y-offset
		 */
		void setOff(const Pos &pos) {
			_off = pos;
		}
		/**
		 * @return the minimum offset in the window at which we can draw
		 */
		Pos getMinOff() const {
			return _minoff;
		}
		/**
		 * Sets the minimum offset in the window, i.e. the beginning where we can draw
		 *
		 * @param pos the offset
		 */
		void setMinOff(const Pos &pos) {
			_minoff = pos;
		}
		/**
		 * Sets the size of the paint-area
		 *
		 * @param size the new size
		 */
		void setSize(const Size &size);

		// used internally
		gsize_t getDim(gpos_t off,gsize_t size,gsize_t max);

		// no cloning
		Graphics(const Graphics &g);
		Graphics &operator=(const Graphics &g);

	protected:
		GraphicsBuffer *_buf;
		// the minimum offset in the window, i.e. the beginning where we can draw
		Pos _minoff;
		// the offset of the control in the window
		Pos _off;
		// the size of the control in the window (i.e. the size of the area that can be painted)
		Size _size;
		// current color
		Color::color_type _col;
		Color _colInst;
		// current font
		Font _font;
	};
}
