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

#ifndef EVENT_H_
#define EVENT_H_

#include <esc/common.h>
#include <esc/keycodes.h>
#include <ctype.h>
#include <ostream>

namespace gui {
	/**
	 * Represents a mouse-event, i.e. a mouse-move or click or similar.
	 */
	class MouseEvent {
		friend std::ostream &operator<<(std::ostream &s,const MouseEvent &e);

	public:
		// the types
		static const uchar MOUSE_MOVED		= 0;
		static const uchar MOUSE_PRESSED	= 1;
		static const uchar MOUSE_RELEASED	= 2;

		// the different buttons
		static const uchar BUTTON1_MASK	= 0x1;
		static const uchar BUTTON2_MASK	= 0x2;
		static const uchar BUTTON3_MASK	= 0x4;

	public:
		/**
		 * Constructor
		 *
		 * @param type the event-type (MOUSE_*)
		 * @param movedx the amount to move in x-direction
		 * @param movedy the amount to move in y-direction
		 * @param x the mouse-x-position
		 * @param y the mouse-y-position
		 * @param buttons the button-state (BUTTON*)
		 */
		MouseEvent(uchar type,short movedx,short movedy,gpos_t x,gpos_t y,uchar buttons)
			: _type(type), _movedx(movedx), _movedy(movedy), _x(x), _y(y), _buttons(buttons) {
		};
		/**
		 * Copy-constructor
		 *
		 * @param e the event to copy
		 */
		MouseEvent(const MouseEvent &e)
			: _type(e._type), _movedx(e._movedx), _movedy(e._movedy), _x(e._x), _y(e._y),
			_buttons(e._buttons) {
		};
		/**
		 * Destructor
		 */
		~MouseEvent() {
		};

		/**
		 * Assignment-operator
		 */
		MouseEvent &operator=(const MouseEvent &e) {
			_type = e._type;
			_movedx = e._movedx;
			_movedy = e._movedy;
			_x = e._x;
			_y = e._y;
			_buttons = e._buttons;
			return *this;
		}

		/**
		 * @return the event-type (MOUSE_*)
		 */
		inline uchar getType() const {
			return _type;
		};
		/**
		 * @return the mouse-x-position
		 */
		inline gpos_t getX() const {
			return _x;
		};
		/**
		 * @return the mouse-y-position
		 */
		inline gpos_t getY() const {
			return _y;
		};
		/**
		 * @return the mouse-x-movement
		 */
		inline short getXMovement() const {
			return _movedx;
		};
		/**
		 * @return the mouse-y-movement
		 */
		inline short getYMovement() const {
			return _movedy;
		};
		/**
		 * @return true if button 1 is pressed
		 */
		inline bool isButton1Down() const {
			return _buttons & BUTTON1_MASK;
		};
		/**
		 * @return true if button 2 is pressed
		 */
		inline bool isButton2Down() const {
			return _buttons & BUTTON2_MASK;
		};
		/**
		 * @return true if button 3 is pressed
		 */
		inline bool isButton3Down() const {
			return _buttons & BUTTON3_MASK;
		};

	private:
		uchar _type;
		short _movedx;
		short _movedy;
		gpos_t _x;
		gpos_t _y;
		uchar _buttons;
	};

	/**
	 * Represents a key-event, i.e. a key-press or release
	 */
	class KeyEvent {
		friend std::ostream &operator<<(std::ostream &s,const KeyEvent &e);

	public:
		// the event-types
		static const uchar KEY_PRESSED		= 0;
		static const uchar KEY_RELEASED		= 1;

	public:
		/**
		 * Constructor
		 *
		 * @param type the event-type (KEY_*)
		 * @param keycode the keycode
		 * @param character the character, depending on the set keymap
		 * @param modifier the enabled modifier
		 */
		KeyEvent(uchar type,uchar keycode,char character,uchar modifier)
			: _type(type), _keycode(keycode), _character(character), _modifier(modifier) {
		};
		/**
		 * Copy-constructor
		 *
		 * @param e the event to copy
		 */
		KeyEvent(const KeyEvent &e)
			: _type(e._type), _keycode(e._keycode), _character(e._character),
			_modifier(e._modifier) {
		};
		/**
		 * Destructor
		 */
		~KeyEvent() {
		};

		/**
		 * Assignment-operator
		 */
		KeyEvent &operator=(const KeyEvent &e) {
			_type = e._type;
			_keycode = e._keycode;
			_character = e._character;
			_modifier = e._modifier;
			return *this;
		};

		/**
		 * @return the event-type (KEY_*)
		 */
		inline uchar getType() const {
			return _type;
		};
		/**
		 * @return the keycode
		 */
		inline uchar getKeyCode() const {
			return _keycode;
		};
		/**
		 * @return the character
		 */
		inline char getCharacter() const {
			return _character;
		};
		/**
		 * whether the character is printable
		 */
		inline bool isPrintable() const {
			return isprint(_character);
		};
		/**
		 * @return whether shift is currently pressed
		 */
		inline bool isShiftDown() const {
			return _modifier & SHIFT_MASK;
		};
		/**
		 * @return whether control is currently pressed
		 */
		inline bool isCtrlDown() const {
			return _modifier & CTRL_MASK;
		};
		/**
		 * @return whether alt is currently pressed
		 */
		inline bool isAltDown() const {
			return _modifier & ALT_MASK;
		};

	private:
		uchar _type;
		uchar _keycode;
		char _character;
		uchar _modifier;
	};

	std::ostream &operator<<(std::ostream &s,const MouseEvent &e);
	std::ostream &operator<<(std::ostream &s,const KeyEvent &e);
}

#endif /* EVENT_H_ */
