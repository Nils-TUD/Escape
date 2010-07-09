/**
 * $Id: uielement.h 511 2010-02-20 23:22:53Z nasmussen $
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
#include <messages.h>
#include <esc/gui/common.h>
#include <esc/gui/graphics.h>
#include <esc/gui/event.h>
#include <esc/gui/mouselistener.h>
#include <esc/gui/keylistener.h>
#include <esc/vector.h>

namespace esc {
	namespace gui {
		class UIElement {
			// window and control need to access _g
			friend class Window;
			friend class Control;

		public:
			UIElement(tCoord x,tCoord y,tSize width,tSize height)
				: _g(NULL), _x(x), _y(y), _width(width), _height(height), _mouseListener(NULL),
				_keyListener(NULL) {
			};
			UIElement(const UIElement &e)
				: _g(NULL), _x(e._x), _y(e._y), _width(e._width), _height(e._height),
				_mouseListener(e._mouseListener), _keyListener(e._keyListener) {
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

			void addMouseListener(MouseListener *l);
			void removeMouseListener(MouseListener *l);

			void addKeyListener(KeyListener *l);
			void removeKeyListener(KeyListener *l);

			virtual void onMouseMoved(const MouseEvent &e);
			virtual void onMouseReleased(const MouseEvent &e);
			virtual void onMousePressed(const MouseEvent &e);
			virtual void onKeyPressed(const KeyEvent &e);
			virtual void onKeyReleased(const KeyEvent &e);

			virtual void repaint();
			virtual void paint(Graphics &g) = 0;
			void requestUpdate();

		protected:
			inline void setX(tCoord x) {
				_x = x;
			};
			inline void setY(tCoord y) {
				_y = y;
			};
			inline void setWidth(tSize width) {
				_width = width;
			};
			inline void setHeight(tSize height) {
				_height = height;
			};
			virtual tWinId getWindowId() const = 0;

		private:
			void notifyListener(const MouseEvent &e);
			void notifyListener(const KeyEvent &e);

		private:
			Graphics *_g;
			tCoord _x;
			tCoord _y;
			tSize _width;
			tSize _height;
			Vector<MouseListener*> *_mouseListener;
			Vector<KeyListener*> *_keyListener;
		};
	}
}

#endif /* UIELEMENT_H_ */
