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

#ifndef UIELEMENT_H_
#define UIELEMENT_H_

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/gui/graphics.h>

namespace esc {
	namespace gui {
		class UIElement {
		public:
			UIElement(tCoord x,tCoord y,tSize width,tSize height)
				: _g(NULL), _x(x), _y(y), _width(width), _height(height) {

			};
			virtual ~UIElement() {

			};

			inline tCoord getX() const {
				return _x;
			};
			inline tCoord getY() const {
				return _y;
			};
			inline tSize getWidth() const {
				return _width;
			};
			inline tSize getHeight() const {
				return _height;
			};

			inline const Graphics *getGraphics() const {
				return _g;
			};

			virtual void paint() = 0;
			void update(tCoord x,tCoord y,tSize width,tSize height);

		protected:
			inline void setX(tCoord x) {
				_x = x;
			};
			inline void setY(tCoord y) {
				_y = y;
			};

		protected:
			Graphics *_g;

		private:
			tCoord _x;
			tCoord _y;
			tSize _width;
			tSize _height;
		};
	}
}

#endif /* UIELEMENT_H_ */
