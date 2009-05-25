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
#include <esc/messages.h>
#include <esc/gui/common.h>
#include <esc/gui/graphics.h>
#include <esc/gui/event.h>

namespace esc {
	namespace gui {
		class UIElement {
			// window and control need to access _g
			friend class Window;
			friend class Control;

		public:
			UIElement(tCoord x,tCoord y,tSize width,tSize height)
				: _g(NULL), _x(x), _y(y), _width(width), _height(height) {
			};
			UIElement(const UIElement &e)
				: _g(NULL), _x(e._x), _y(e._y), _width(e._width), _height(e._height) {
			}
			virtual ~UIElement() {
				delete _g;
			};
			UIElement &operator=(const UIElement &e);

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

			inline Graphics *getGraphics() const {
				return _g;
			};

			virtual void onMouseMoved(const MouseEvent &e) = 0;
			virtual void onMouseReleased(const MouseEvent &e) = 0;
			virtual void onMousePressed(const MouseEvent &e) = 0;
			virtual void onKeyPressed(const KeyEvent &e) = 0;
			virtual void onKeyReleased(const KeyEvent &e) = 0;

			virtual void repaint();
			virtual void paint(Graphics &g) = 0;
			void requestUpdate();
			void update(tCoord x,tCoord y,tSize width,tSize height);

		protected:
			inline void setX(tCoord x) {
				_x = x;
			};
			inline void setY(tCoord y) {
				_y = y;
			};
			virtual tWinId getWindowId() const = 0;

		private:
			Graphics *_g;
			tCoord _x;
			tCoord _y;
			tSize _width;
			tSize _height;
		};
	}
}

#endif /* UIELEMENT_H_ */
