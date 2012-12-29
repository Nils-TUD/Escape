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

#ifndef WINDOWLISTENER_H_
#define WINDOWLISTENER_H_

#include <esc/common.h>
#include <string>

namespace gui {
	class WindowListener {
	public:
		/**
		 * Destructor
		 */
		virtual ~WindowListener() {
		};

		/**
		 * Is called as soon as a window is created
		 *
		 * @param id the window-id
		 * @param title the window-title
		 */
		virtual void onWindowCreated(gwinid_t id,const std::string& title) = 0;

		/**
		 * Is called as soon as the active window changes
		 *
		 * @param id the active window-id
		 */
		virtual void onWindowActive(gwinid_t id) = 0;

		/**
		 * Is called as soon as a window is destroyed.
		 *
		 * @param id the window-id
		 */
		virtual void onWindowDestroyed(gwinid_t id) = 0;
	};
}

#endif /* WINDOWLISTENER_H_ */
