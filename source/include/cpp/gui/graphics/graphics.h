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

#pragma once

#include <esc/common.h>
#include <gui/graphics/graphicsbuffer.h>
#include <gui/graphics/font.h>
#include <gui/graphics/color.h>
#include <gui/graphics/size.h>
#include <gui/graphics/pos.h>
#include <gui/graphics/rectangle.h>
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
		friend class BitmapImage;

	private:
		static const int OUT_TOP		= 1;
		static const int OUT_RIGHT		= 2;
		static const int OUT_BOTTOM		= 4;
		static const int OUT_LEFT		= 8;

	public:
		/**
		 * Constructor
		 *
		 * @param buf the graphics-buffer for the window
		 * @param size the size of the control
		 */
		Graphics(GraphicsBuffer *buf,const Size &size)
			: _buf(buf), _minoff(), _off(), _size(size), _col(0), _colInst(0), _minx(0),_miny(0),
			  _maxx(size.width - 1), _maxy(size.height - 1), _font() {
		}
		/**
		 * Destructor
		 */
		virtual ~Graphics() {
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
		virtual void moveRows(const Pos &pos,const Size &size,int up);
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
		virtual void moveCols(const Pos &pos,const Size &size,int left);
		void moveCols(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int left) {
			moveCols(Pos(x,y),Size(width,height),left);
		}

		/**
		 * Draws the given character at given position
		 *
		 * @param pos the position
		 * @param c the character
		 */
		virtual void drawChar(const Pos &pos,char c);
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
		virtual void drawString(const Pos &pos,const std::string &str);
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
		virtual void drawString(const Pos &pos,const std::string &str,size_t start,size_t count);
		void drawString(gpos_t x,gpos_t y,const std::string &str,size_t start,size_t count) {
			drawString(Pos(x,y),str,start,count);
		}

		/**
		 * Draws a line from p0 to pn
		 *
		 * @param p0 first coordinate
		 * @param pn last coordinate
		 */
		virtual void drawLine(const Pos &p0,const Pos &pn);
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
		virtual void drawVertLine(gpos_t x,gpos_t y1,gpos_t y2);

		/**
		 * Draws a horizontal line (optimized compared to drawLine)
		 *
		 * @param y the y-coordinate
		 * @param x1 the first x-coordinate
		 * @param x2 the second x-coordinate
		 */
		virtual void drawHorLine(gpos_t y,gpos_t x1,gpos_t x2);

		/**
		 * Draws a rectangle
		 *
		 * @param pos the position
		 * @param size the size
		 */
		virtual void drawRect(const Pos &pos,const Size &size);
		void drawRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
			drawRect(Pos(x,y),Size(width,height));
		}

		/**
		 * Fills a rectangle
		 *
		 * @param pos the position
		 * @param size the size
		 */
		virtual void fillRect(const Pos &pos,const Size &size);
		void fillRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
			fillRect(Pos(x,y),Size(width,height));
		}

		/**
		 * @return the x-offset of the control in the window
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
			return _buf->getBuffer();
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
		virtual void doSetPixel(gpos_t x,gpos_t y) = 0;
		/**
		 * Adds the given position to the dirty region
		 */
		void updateMinMax(const Pos &pos) {
			if(pos.x > _maxx)
				_maxx = pos.x;
			if(pos.x < _minx)
				_minx = pos.x;
			if(pos.y > _maxy)
				_maxy = pos.y;
			if(pos.y < _miny)
				_miny = pos.y;
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
		 * @return the minimum y-offset in the window at which we can draw
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
		// dirty region
		gpos_t _minx,_miny,_maxx,_maxy;
		// current font
		Font _font;
	};
}
