/**
 * $Id: graphics.h 965 2011-07-07 10:56:45Z nasmussen $
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

#ifndef GRAPHICSBUFFER_H_
#define GRAPHICSBUFFER_H_

#include <esc/common.h>
#include <stdlib.h>

namespace gui {
	class Window;

	/**
	 * The graphics-buffer holds an array of a specific size and provides access to it. This is
	 * used as a buffer for the complete window. All graphics-instances will share the graphics-
	 * buffer instance from their window.
	 */
	class GraphicsBuffer {
		friend class Window;

	public:
		/**
		 * Constructor
		 *
		 * @param x the x-coordinate of the window
		 * @param y the y-coordinate of the window
		 * @param width width of the window
		 * @param height height of the window
		 * @param bpp the used color-depth
		 */
		GraphicsBuffer(Window *win,gpos_t x,gpos_t y,gsize_t width,gsize_t height,gcoldepth_t bpp)
			: _win(win), _x(x), _y(y), _width(width), _height(height), _bpp(bpp), _pixels(NULL) {
			allocBuffer();
		};
		/**
		 * Destructor
		 */
		virtual ~GraphicsBuffer() {
			free(_pixels);
		};

		/**
		 * @return the x-position of the buffer (global)
		 */
		inline gpos_t getX() const {
			return _x;
		};
		/**
		 * @return the y-position of the buffer (global)
		 */
		inline gpos_t getY() const {
			return _y;
		};
		/**
		 * @return the width of the buffer
		 */
		inline gsize_t getWidth() const {
			return _width;
		};
		/**
		 * @return the height of the buffer
		 */
		inline gsize_t getHeight() const {
			return _height;
		};

		/**
		 * @return the buffer
		 */
		inline uint8_t *getBuffer() const {
			return _pixels;
		};

		/**
		 * @return the color-depth
		 */
		inline gcoldepth_t getColorDepth() const {
			return _bpp;
		};

		/**
		 * Requests an update for the given region
		 */
		void requestUpdate(gpos_t x,gpos_t y,gsize_t width,gsize_t height);
		/**
		 * Updates the given region: writes to the shared-mem offered by vesa and notifies vesa
		 */
		void update(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

	private:
		// no cloning
		GraphicsBuffer(const GraphicsBuffer &g);
		GraphicsBuffer &operator=(const GraphicsBuffer &g);

		/**
		 * Sets the coordinates for this buffer
		 */
		void moveTo(gpos_t x,gpos_t y);
		/**
		 * Sets the dimensions for this buffer
		 */
		void resizeTo(gsize_t width,gsize_t height);
		/**
		 * Allocates _pixels
		 */
		void allocBuffer();
		/**
		 * Notifies vesa that the given region has changed
		 */
		void notifyVesa(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

	private:
		// the window instance the buffer belongs to
		Window *_win;
		// the position of the window on the screen
		gpos_t _x,_y;
		// size of the window
		gsize_t _width;
		gsize_t _height;
		// used color-depth
		gcoldepth_t _bpp;
		// buffer for this window; controls use this, too (don't have their own)
		uint8_t *_pixels;
	};
}

#endif /* GRAPHICSBUFFER_H_ */
