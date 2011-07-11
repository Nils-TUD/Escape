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

#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include <esc/common.h>
#include <gui/graphicsbuffer.h>
#include <gui/font.h>
#include <gui/color.h>
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
		 * @param width the width of the control
		 * @param height the height of the control
		 */
		Graphics(GraphicsBuffer *buf,gsize_t width,gsize_t height)
			: _buf(buf), _minoffx(0), _minoffy(0), _offx(0), _offy(0), _width(width), _height(height),
			  _col(0), _colInst(Color(0)),
			  _minx(0),_miny(0), _maxx(width - 1), _maxy(height - 1),
			  _font(Font()) {
		};
		/**
		 * Destructor
		 */
		virtual ~Graphics() {
		};

		/**
		 * @return the current font
		 */
		inline const Font &getFont() const {
			return _font;
		};

		/**
		 * @return the current color
		 */
		inline Color getColor() const {
			return _colInst;
		};

		/**
		 * Sets the color
		 *
		 * @param col the new color
		 */
		inline void setColor(const Color &col) {
			_col = col.toCurMode();
			_colInst = col;
		};

		/**
		 * Sets the pixel to the current color
		 *
		 * @param x the x-coordinate
		 * @param y the y-coordinate
		 */
		inline void setPixel(gpos_t x,gpos_t y) {
			if(!validatePoint(x,y))
				return;
			updateMinMax(x,y);
			doSetPixel(x,y);
		};

		/**
		 * Moves <height> rows of <width> pixels at <x>,<y> up by <up> rows.
		 * If <up> is negative, it moves them down.
		 *
		 * @param x the x-coordinate where to start
		 * @param y the y-coordinate where to start
		 * @param width the width of a row
		 * @param height the number of rows to move
		 * @param up amount to move up / down
		 */
		virtual void moveRows(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int up);

		/**
		 * Moves <width> cols of <height> pixels at <x>,<y> left by <left> cols.
		 * If <left> is negative, it moves them right.
		 *
		 * @param x the x-coordinate where to start
		 * @param y the y-coordinate where to start
		 * @param width the number of cols to move
		 * @param height the height of a col
		 * @param left amount to move left / right
		 */
		virtual void moveCols(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int left);

		/**
		 * Draws the given character at given position
		 *
		 * @param x the x-coordinate
		 * @param y the y-coordinate
		 * @param c the character
		 */
		virtual void drawChar(gpos_t x,gpos_t y,char c);

		/**
		 * Draws the given string at the given position. Note that the position is the top
		 * left of the first character.
		 *
		 * @param x the x-coordinate
		 * @param y the y-coordinate
		 * @param str the string
		 */
		virtual void drawString(gpos_t x,gpos_t y,const string &str);

		/**
		 * Draws the given part of the given string at the given position. Note that the
		 * position is the top left of the first character.
		 *
		 * @param x the x-coordinate
		 * @param y the y-coordinate
		 * @param str the string
		 * @param start the start-position in the string
		 * @param count the number of characters
		 */
		virtual void drawString(gpos_t x,gpos_t y,const string &str,size_t start,size_t count);

		/**
		 * Draws a line from (x0,y0) to (xn,yn)
		 *
		 * @param x0 first x-coordinate
		 * @param y0 first y-coordinate
		 * @param xn last x-coordinate
		 * @param yn last y-coordinate
		 */
		virtual void drawLine(gpos_t x0,gpos_t y0,gpos_t xn,gpos_t yn);

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
		 * @param x the x-coordinate
		 * @param y the y-coordinate
		 * @param width the width
		 * @param height the height
		 */
		virtual void drawRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

		/**
		 * Fills a rectangle
		 *
		 * @param x the x-coordinate
		 * @param y the y-coordinate
		 * @param width the width
		 * @param height the height
		 */
		virtual void fillRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

		/**
		 * @return the x-offset of the control in the window
		 */
		inline gpos_t getX() const {
			return _offx;
		};
		/**
		 * @return the y-offset of the control in the window
		 */
		inline gpos_t getY() const {
			return _offy;
		};

		/**
		 * @return the width of the paintable area
		 */
		inline gsize_t getWidth() const {
			return _width;
		};
		/**
		 * @return the height of the paintable area
		 */
		inline gsize_t getHeight() const {
			return _height;
		};

	protected:
		/**
		 * Sets a pixel (without check)
		 */
		virtual void doSetPixel(gpos_t x,gpos_t y) = 0;
		/**
		 * Adds the given position to the dirty region
		 */
		inline void updateMinMax(gpos_t x,gpos_t y) {
			if(x > _maxx)
				_maxx = x;
			if(x < _minx)
				_minx = x;
			if(y > _maxy)
				_maxy = y;
			if(y < _miny)
				_miny = y;
		};
		/**
		 * Requests an update for the dirty region
		 */
		void requestUpdate();
		/**
		 * Updates the given region: writes to the shared-mem offered by vesa and notifies vesa
		 */
		void update(gpos_t x,gpos_t y,gsize_t width,gsize_t height);
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

	private:
		/**
		 * @return the graphics-buffer
		 */
		inline GraphicsBuffer* getBuffer() const {
			return _buf;
		};
		/**
		 * Sets the offset of the control in the window
		 *
		 * @param x the x-offset
		 * @param y the y-offset
		 */
		inline void setOff(gpos_t x,gpos_t y) {
			_offx = x;
			_offy = y;
		};
		/**
		 * @return the minimum x-offset in the window at which we can draw
		 */
		inline gpos_t getMinX() const {
			return _minoffx;
		};
		/**
		 * @return the minimum y-offset in the window at which we can draw
		 */
		inline gpos_t getMinY() const {
			return _minoffy;
		};
		/**
		 * Sets the minimum offset in the window, i.e. the beginning where we can draw
		 *
		 * @param x the x-offset
		 * @param y the y-offset
		 */
		inline void setMinOff(gpos_t x,gpos_t y) {
			_minoffx = x;
			_minoffy = y;
		};
		/**
		 * Sets the width of the paint-area (unchecked!)
		 *
		 * @param width the new value
		 */
		inline void setWidth(gsize_t width) {
			_width = width;
		};
		/**
		 * Sets the height of the paint-area (unchecked!)
		 *
		 * @param height the new value
		 */
		inline void setHeight(gsize_t height) {
			_height = height;
		};
		/**
		 * Sets the size of the control (checked)
		 *
		 * @param x the x-position of the control relative to the parent
		 * @param y the y-position of the control relative to the parent
		 * @param width the new value
		 * @param height the new value
		 * @param pwidth the width of the parent. the child can't cross that
		 * @param pheight the height of the parent. the child can't cross that
		 */
		void setSize(gpos_t x,gpos_t y,gsize_t width,gsize_t height,gsize_t pwidth,gsize_t pheight);

		// used internally
		gsize_t getDim(gpos_t off,gsize_t size,gsize_t max);

		// no cloning
		Graphics(const Graphics &g);
		Graphics &operator=(const Graphics &g);

	protected:
		GraphicsBuffer *_buf;
		// the minimum offset in the window, i.e. the beginning where we can draw
		gpos_t _minoffx;
		gpos_t _minoffy;
		// the offset of the control in the window
		gpos_t _offx;
		gpos_t _offy;
		// the size of the control in the window (i.e. the size of the area that can be painted)
		gsize_t _width;
		gsize_t _height;
		// current color
		Color::color_type _col;
		Color _colInst;
		// dirty region
		gpos_t _minx,_miny,_maxx,_maxy;
		// current font
		Font _font;
	};
}

#endif /* GRAPHICS_H_ */
