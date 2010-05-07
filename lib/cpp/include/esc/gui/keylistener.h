/**
 * $Id: keylistener.h 251 2009-08-23 12:16:34Z nasmussen $
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

#ifndef KEYLISTENER_H_
#define KEYLISTENER_H_

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/gui/event.h>

namespace esc {
	namespace gui {
		class KeyListener {
		public:
			KeyListener() {
			};
			virtual ~KeyListener() {
			};

			virtual void keyPressed(const KeyEvent &e) = 0;
			virtual void keyReleased(const KeyEvent &e) = 0;

		private:
			// no copying
			KeyListener(const KeyListener &l);
			KeyListener &operator=(const KeyListener &l);
		};
	}
}

#endif /* KEYLISTENER_H_ */
