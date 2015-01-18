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

#include <gui/graphics/pos.h>
#include <sys/common.h>
#include <sys/keycodes.h>
#include <ctype.h>
#include <ostream>

namespace gui {
	/**
	 * Represents a mouse-event, i.e. a mouse-move or click or similar.
	 */
	class MouseEvent {
		friend std::ostream &operator<<(std::ostream &s,const MouseEvent &e);

	public:
		typedef uchar event_type;
		typedef uchar button_type;

	public:
		// the types
		static const event_type MOUSE_MOVED		= 0;
		static const event_type MOUSE_PRESSED	= 1;
		static const event_type MOUSE_RELEASED	= 2;
		static const event_type MOUSE_WHEEL		= 3;

		// the different buttons
		static const button_type BUTTON1_MASK	= 0x1;
		static const button_type BUTTON2_MASK	= 0x2;
		static const button_type BUTTON3_MASK	= 0x4;

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
		MouseEvent(event_type type,short movedx,short movedy,short movedz,
		           const Pos &pos,button_type buttons)
			: _type(type), _movedx(movedx), _movedy(movedy), _movedz(movedz), _pos(pos),
			  _buttons(buttons) {
		}

		/**
		 * @return the event-type (MOUSE_*)
		 */
		event_type getType() const {
			return _type;
		}
		/**
		 * @return the mouse-position
		 */
		Pos getPos() const {
			return _pos;
		}
		/**
		 * @return the mouse-x-movement
		 */
		short getXMovement() const {
			return _movedx;
		}
		/**
		 * @return the mouse-y-movement
		 */
		short getYMovement() const {
			return _movedy;
		}
		/**
		 * @return the mousewheel movement
		 */
		short getWheelMovement() const {
			return _movedz;
		}
		/**
		 * @return the button-mask
		 */
		button_type getButtonMask() const {
			return _buttons;
		}
		/**
		 * @return true if button 1 is pressed
		 */
		bool isButton1Down() const {
			return _buttons & BUTTON1_MASK;
		}
		/**
		 * @return true if button 2 is pressed
		 */
		bool isButton2Down() const {
			return _buttons & BUTTON2_MASK;
		}
		/**
		 * @return true if button 3 is pressed
		 */
		bool isButton3Down() const {
			return _buttons & BUTTON3_MASK;
		}

	private:
		event_type _type;
		short _movedx;
		short _movedy;
		short _movedz;
		Pos _pos;
		button_type _buttons;
	};

	/**
	 * Represents a key-event, i.e. a key-press or release
	 */
	class KeyEvent {
		friend std::ostream &operator<<(std::ostream &s,const KeyEvent &e);

	public:
		typedef uchar event_type;
		typedef uchar keycode_type;
		typedef uchar modifier_type;

	public:
		// the event-types
		static const event_type KEY_PRESSED		= 0;
		static const event_type KEY_RELEASED	= 1;

	public:
		/**
		 * Constructor
		 *
		 * @param type the event-type (KEY_*)
		 * @param keycode the keycode
		 * @param character the character, depending on the set keymap
		 * @param modifier the enabled modifier
		 */
		KeyEvent(event_type type,keycode_type keycode,char character,modifier_type modifier)
			: _type(type), _keycode(keycode), _character(character), _modifier(modifier) {
		}

		/**
		 * @return the event-type (KEY_*)
		 */
		event_type getType() const {
			return _type;
		}
		/**
		 * @return the keycode
		 */
		keycode_type getKeyCode() const {
			return _keycode;
		}
		/**
		 * @return the character
		 */
		char getCharacter() const {
			return _character;
		}
		/**
		 * whether the character is printable
		 */
		bool isPrintable() const {
			return isprint(_character);
		}
		/**
		 * @return whether shift is currently pressed
		 */
		bool isShiftDown() const {
			return _modifier & SHIFT_MASK;
		}
		/**
		 * @return whether control is currently pressed
		 */
		bool isCtrlDown() const {
			return _modifier & CTRL_MASK;
		}
		/**
		 * @return whether alt is currently pressed
		 */
		bool isAltDown() const {
			return _modifier & ALT_MASK;
		}

	private:
		event_type _type;
		keycode_type _keycode;
		char _character;
		modifier_type _modifier;
	};

	std::ostream &operator<<(std::ostream &s,const MouseEvent &e);
	std::ostream &operator<<(std::ostream &s,const KeyEvent &e);
}
