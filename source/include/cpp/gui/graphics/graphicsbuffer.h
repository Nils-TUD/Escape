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
#include <gui/graphics/size.h>
#include <gui/graphics/pos.h>
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
		 * @param pos the coordinates of the window
		 * @param width width of the window
		 * @param height height of the window
		 * @param bpp the used color-depth
		 */
		GraphicsBuffer(Window *win,const Pos &pos,const Size &size,gcoldepth_t bpp)
			: _win(win), _pos(pos), _size(size), _bpp(bpp), _pixels(NULL) {
			allocBuffer();
		}
		/**
		 * Destructor
		 */
		virtual ~GraphicsBuffer() {
			free(_pixels);
		}

		/**
		 * @return the position of the buffer (global)
		 */
		Pos getPos() const {
			return _pos;
		}
		/**
		 * @return the size of the buffer
		 */
		Size getSize() const {
			return _size;
		}

		/**
		 * @return the window this buffer belongs to
		 */
		Window *getWindow() const {
			return _win;
		}

		/**
		 * @return the buffer
		 */
		uint8_t *getBuffer() const {
			return _pixels;
		}

		/**
		 * @return the color-depth
		 */
		gcoldepth_t getColorDepth() const {
			return _bpp;
		}

		/**
		 * Requests an update for the given region
		 */
		void requestUpdate(const Pos &pos,const Size &size);
		/**
		 * Updates the given region: writes to the shared-mem offered by vesa and notifies vesa
		 */
		void update(const Pos &pos,const Size &size);

	private:
		// no cloning
		GraphicsBuffer(const GraphicsBuffer &g);
		GraphicsBuffer &operator=(const GraphicsBuffer &g);

		/**
		 * Sets the coordinates for this buffer
		 */
		void moveTo(const Pos &pos);
		/**
		 * Sets the dimensions for this buffer
		 */
		void resizeTo(const Size &size);
		/**
		 * Allocates _pixels
		 */
		void allocBuffer();
		/**
		 * Notifies vesa that the given region has changed
		 */
		void notifyVesa(const Pos &pos,const Size &size);

	private:
		// the window instance the buffer belongs to
		Window *_win;
		// the position of the window on the screen
		Pos _pos;
		// size of the window
		Size _size;
		// used color-depth
		gcoldepth_t _bpp;
		// buffer for this window; controls use this, too (don't have their own)
		uint8_t *_pixels;
	};
}
