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

#ifndef MOUSELISTENER_H_
#define MOUSELISTENER_H_

#include <esc/common.h>
#include <gui/event.h>

namespace gui {
	/**
	 * The abstract class to listen for mouseevents
	 */
	class MouseListener {
	public:
		/**
		 * Empty constructor
		 */
		MouseListener() {
		};
		/**
		 * Destructor
		 */
		virtual ~MouseListener() {
		};

		/**
		 * Abstract method that is called as soon as the mouse has been moved
		 *
		 * @param e the mouse-event
		 */
		virtual void mouseMoved(const MouseEvent &e) = 0;
		/**
		 * Abstract method that is called as soon as a mouse-button has been released
		 *
		 * @param e the mouse-event
		 */
		virtual void mousePressed(const MouseEvent &e) = 0;
		/**
		 * Abstract method that is called as soon as a mouse-button has been pressed
		 *
		 * @param e the mouse-event
		 */
		virtual void mouseReleased(const MouseEvent &e) = 0;

	private:
		// no copying
		MouseListener(const MouseListener &l);
		MouseListener &operator=(const MouseListener &l);
	};
}

#endif /* MOUSELISTENER_H_ */
