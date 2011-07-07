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
#include <esc/io.h>
#include <esc/proc.h>
#include <esc/mem.h>
#include <gui/application.h>
#include <gui/font.h>
#include <gui/color.h>
#include <esc/messages.h>
#include <stdlib.h>
#include <string>
#include <ostream>

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

	public:
		/**
		 * Constructor for controls
		 *
		 * @param g the graphics-object of the window
		 * @param x the x-coordinate of the control
		 * @param y the y-coordinate of the control
		 */
		Graphics(Graphics &g,gpos_t x,gpos_t y);

		/**
		 * Constructor for windows
		 *
		 * @param x the x-coordinate of the window
		 * @param y the y-coordinate of the window
		 * @param width width of the window
		 * @param height height of the window
		 * @param bpp the used color-depth
		 */
		Graphics(gpos_t x,gpos_t y,gsize_t width,gsize_t height,gcoldepth_t bpp);

		/**
		 * Destructor
		 */
		virtual ~Graphics();

		/**
		 * @return the current font
		 */
		inline Font getFont() const {
			return _font;
		};

		/**
		 * @return the color-depth
		 */
		inline gcoldepth_t getColorDepth() const {
			return _bpp;
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
			x %= _width;
			y %= _height;
			updateMinMax(x,y);
			doSetPixel(x,y);
		};

		/**
		 * Moves <height> lines at <y> up by <up> lines. If up is negative, it moves them
		 * down
		 *
		 * @param y the y-coordinate where to start
		 * @param height the number of lines to move
		 * @param up amount to move up / down
		 */
		virtual void moveLines(gpos_t y,gsize_t height,int up);

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

	private:
		// prevent the compiler from generating copy-constructor and assignment-operator
		// by declaring them with out definition
		Graphics(const Graphics &g);
		Graphics &operator=(const Graphics &g);

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
		 * Sets the coordinates for this control / window
		 */
		void moveTo(gpos_t x,gpos_t y);
		/**
		 * Sets the dimensions for this control / window
		 */
		void resizeTo(gsize_t width,gsize_t height);
		/**
		 * Requests an update for the dirty region
		 */
		void requestUpdate(gwinid_t winid);
		/**
		 * Updates the given region: writes to the shared-mem offered by vesa and notifies vesa
		 */
		void update(gpos_t x,gpos_t y,gsize_t width,gsize_t height);
		/**
		 * Notifies vesa that the given region has changed
		 */
		void notifyVesa(gpos_t x,gpos_t y,gsize_t width,gsize_t height);
		/**
		 * Validates the given position
		 */
		void validatePos(gpos_t &x,gpos_t &y);
		/**
		 * Validates the given parameters
		 */
		void validateParams(gpos_t &x,gpos_t &y,gsize_t &width,gsize_t &height);

	private:
		/**
		 * Allocates _pixels
		 */
		void allocBuffer();

	protected:
		// for controls: the offset of the control in the window (otherwise 0)
		gpos_t _offx,_offy;
		// the position of the window on the screen
		gpos_t _x,_y;
		// size of the window
		gsize_t _width;
		gsize_t _height;
		// used color-depth
		gcoldepth_t _bpp;
		// current color
		Color::color_type _col;
		Color _colInst;
		// dirty region
		gpos_t _minx,_miny,_maxx,_maxy;
		// buffer for this window; controls use this, too (don't have their own)
		uint8_t *_pixels;
		// current font
		Font _font;
		// for controls: the graphics-instance of the window
		Graphics *_owner;
	};
}

#endif /* GRAPHICS_H_ */
