/**
 * $Id: mouselistener.h 251 2009-08-23 12:16:34Z nasmussen $
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

#ifndef MOUSELISTENER_H_
#define MOUSELISTENER_H_

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/gui/event.h>

namespace esc {
	namespace gui {
		class MouseListener {
		public:
			MouseListener() {
			};
			virtual ~MouseListener() {
			};

			virtual void mouseMoved(const MouseEvent &e) = 0;
			virtual void mouseReleased(const MouseEvent &e) = 0;
			virtual void mousePressed(const MouseEvent &e) = 0;

		private:
			// no copying
			MouseListener(const MouseListener &l);
			MouseListener &operator=(const MouseListener &l);
		};
	}
}

#endif /* MOUSELISTENER_H_ */
