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
		class MouseEvent {
		public:
			static const u8 BUTTON1_MASK = 0x1;
			static const u8 BUTTON2_MASK = 0x2;
			static const u8 BUTTON3_MASK = 0x4;

		public:
			MouseEvent(s16 movedx,s16 movedy,tCoord x,tCoord y,u8 buttons)
				: _movedx(movedx), _movedy(movedy), _x(x), _y(y), _buttons(buttons) {

			};
			MouseEvent(const MouseEvent &e)
				: _movedx(e._movedx), _movedy(e._movedy), _x(e._x), _y(e._y), _buttons(e._buttons) {

			};
			virtual ~MouseEvent() {

			};

			inline tCoord getX() const {
				return _x;
			};
			inline tCoord getY() const {
				return _y;
			};
			inline s16 getXMovement() const {
				return _movedx;
			};
			inline s16 getYMovement() const {
				return _movedy;
			};
			inline bool isButton1Down() const {
				return _buttons & BUTTON1_MASK;
			};
			inline bool isButton2Down() const {
				return _buttons & BUTTON2_MASK;
			};
			inline bool isButton3Down() const {
				return _buttons & BUTTON3_MASK;
			};

		private:
			s16 _movedx;
			s16 _movedy;
			tCoord _x;
			tCoord _y;
			u8 _buttons;
		};

		class UIElement {
		public:
			UIElement(tCoord x,tCoord y,tSize width,tSize height)
				: _g(NULL), _x(x), _y(y), _width(width), _height(height) {

			};
			virtual ~UIElement() {
				delete _g;
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

			inline Graphics *getGraphics() const {
				return _g;
			};

			virtual void onMouseMoved(const MouseEvent &e) = 0;
			virtual void onMouseReleased(const MouseEvent &e) = 0;
			virtual void onMousePressed(const MouseEvent &e) = 0;

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
