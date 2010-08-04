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

#ifndef EVENT_H_
#define EVENT_H_

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/keycodes.h>
#include <ctype.h>
#include <ostream>

namespace esc {
	namespace gui {
		class MouseEvent {
			friend ostream &operator<<(ostream &s,const MouseEvent &e);

		public:
			static const u8 MOUSE_MOVED		= 0;
			static const u8 MOUSE_PRESSED	= 1;
			static const u8 MOUSE_RELEASED	= 2;

			static const u8 BUTTON1_MASK	= 0x1;
			static const u8 BUTTON2_MASK	= 0x2;
			static const u8 BUTTON3_MASK	= 0x4;

		public:
			MouseEvent(u8 type,s16 movedx,s16 movedy,tCoord x,tCoord y,u8 buttons)
				: _type(type), _movedx(movedx), _movedy(movedy), _x(x), _y(y), _buttons(buttons) {
			};
			MouseEvent(const MouseEvent &e)
				: _type(e._type), _movedx(e._movedx), _movedy(e._movedy), _x(e._x), _y(e._y),
				_buttons(e._buttons) {
			};
			~MouseEvent() {

			};

			MouseEvent &operator=(const MouseEvent &e) {
				_type = e._type;
				_movedx = e._movedx;
				_movedy = e._movedy;
				_x = e._x;
				_y = e._y;
				_buttons = e._buttons;
				return *this;
			}

			inline u8 getType() const {
				return _type;
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
			u8 _type;
			s16 _movedx;
			s16 _movedy;
			tCoord _x;
			tCoord _y;
			u8 _buttons;
		};

		class KeyEvent {
			friend ostream &operator<<(ostream &s,const KeyEvent &e);

		public:
			static const u8 KEY_PRESSED		= 0;
			static const u8 KEY_RELEASED	= 1;

		public:
			KeyEvent(u8 type,u8 keycode,char character,u8 modifier)
				: _type(type), _keycode(keycode), _character(character), _modifier(modifier) {
			};
			KeyEvent(const KeyEvent &e)
				: _type(e._type), _keycode(e._keycode), _character(e._character),
				_modifier(e._modifier) {
			};
			~KeyEvent() {
			};

			KeyEvent &operator=(const KeyEvent &e) {
				_type = e._type;
				_keycode = e._keycode;
				_character = e._character;
				_modifier = e._modifier;
				return *this;
			};

			inline u8 getType() const {
				return _type;
			};
			inline u8 getKeyCode() const {
				return _keycode;
			};
			inline char getCharacter() const {
				return _character;
			};
			inline bool isPrintable() const {
				return isprint(_character);
			};
			inline bool isShiftDown() const {
				return _modifier & SHIFT_MASK;
			};
			inline bool isCtrlDown() const {
				return _modifier & CTRL_MASK;
			};
			inline bool isAltDown() const {
				return _modifier & ALT_MASK;
			};

		private:
			u8 _type;
			u8 _keycode;
			char _character;
			u8 _modifier;
		};

		ostream &operator<<(ostream &s,const MouseEvent &e);
		ostream &operator<<(ostream &s,const KeyEvent &e);
	}
}

#endif /* EVENT_H_ */
