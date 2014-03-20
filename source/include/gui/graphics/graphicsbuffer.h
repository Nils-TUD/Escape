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

#include <esc/common.h>
#include <gui/graphics/size.h>
#include <gui/graphics/pos.h>
#include <gui/graphics/rectangle.h>
#include <stdlib.h>

namespace gui {
	class Window;
	class Graphics;
	class UIElement;

	/**
	 * The graphics-buffer holds an array of a specific size and provides access to it. This is
	 * used as a buffer for the complete window. All graphics-instances will share the graphics-
	 * buffer instance from their window.
	 */
	class GraphicsBuffer {
		friend class Window;
		friend class Graphics;
		friend class UIElement;

	public:
		/**
		 * Constructor
		 *
		 * @param pos the coordinates of the window
		 * @param width width of the window
		 * @param height height of the window
		 */
		GraphicsBuffer(Window *win,const Pos &pos,const Size &size)
			: _win(win), _pos(pos), _size(size), _minx(0),_miny(0),
			  _maxx(size.width - 1), _maxy(size.height - 1), _pixels(nullptr) {
		}
		/**
		 * Destructor
		 */
		virtual ~GraphicsBuffer() {
			freeBuffer();
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
		 * @return true if we're ready to paint
		 */
		bool isReady() const {
			return _pixels;
		}

		/**
		 * Requests an update for the given region
		 */
		void requestUpdate(const Pos &pos,const Size &size);

	private:
		// no cloning
		GraphicsBuffer(const GraphicsBuffer &g);
		GraphicsBuffer &operator=(const GraphicsBuffer &g);

		/**
		 * @return the buffer
		 */
		uint8_t *getBuffer() const {
			return _pixels;
		}
		/**
		 * Sets the coordinates for this buffer
		 */
		void moveTo(const Pos &pos);
		/**
		 * Sets the dimensions for this buffer
		 */
		void resizeTo(const Size &size) {
			_size = size;
		}
		/**
		 * Allocates _pixels
		 */
		void allocBuffer();
		/**
		 * Frees _pixels
		 */
		void freeBuffer();
		/**
		 * @return the dirty rectangle
		 */
		Rectangle getDirtyRect() {
			if(_minx > _maxx)
				return Rectangle();
			return Rectangle(Pos(_minx,_miny),Size(_maxx - _minx + 1,_maxy - _miny + 1));
		}
		/**
		 * Marks everything clean
		 */
		void resetDirty() {
			_minx = _size.width - 1;
			_maxx = 0;
			_miny = _size.height - 1;
			_maxy = 0;
		}
		/**
		 * Adds the given position to the dirty region
		 */
		void updateDirty(const Pos &pos) {
			if(pos.x > _maxx)
				_maxx = pos.x;
			if(pos.x < _minx)
				_minx = pos.x;
			if(pos.y > _maxy)
				_maxy = pos.y;
			if(pos.y < _miny)
				_miny = pos.y;
		}

	private:
		// the window instance the buffer belongs to
		Window *_win;
		// the position of the window on the screen
		Pos _pos;
		// size of the window
		Size _size;
		// dirty region
		gpos_t _minx,_miny,_maxx,_maxy;
		// buffer for this window; controls use this, too (don't have their own)
		uint8_t *_pixels;
	};
}
