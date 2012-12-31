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

#ifndef KEYLISTENER_H_
#define KEYLISTENER_H_

#include <esc/common.h>
#include <gui/event/event.h>

namespace gui {
	class UIElement;

	/**
	 * The abstract class to listen for keyevents
	 */
	class KeyListener {
	public:
		KeyListener() {
		};
		virtual ~KeyListener() {
		};

		/**
		 * Is called as soon as a key has been pressed
		 *
		 * @param el the ui-element that received this event
		 * @param e the key-event
		 */
		virtual void keyPressed(UIElement &,const KeyEvent &) {};

		/**
		 * Is called as soon as a key has been released
		 *
		 * @param el the ui-element that received this event
		 * @param e the key-event
		 */
		virtual void keyReleased(UIElement &,const KeyEvent &) {};

	private:
		// no copying
		KeyListener(const KeyListener &l);
		KeyListener &operator=(const KeyListener &l);
	};
}

#endif /* KEYLISTENER_H_ */
