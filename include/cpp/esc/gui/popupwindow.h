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

#ifndef POPUPWINDOW_H_
#define POPUPWINDOW_H_

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/gui/window.h>

namespace esc {
	namespace gui {
		class PopupWindow : public Window {
		public:
			PopupWindow(tCoord x,tCoord y,tSize width,tSize height)
				: Window("",x,y,width,height,STYLE_POPUP) {
			};
			PopupWindow(const PopupWindow &w)
				: Window(w) {
			};
			virtual ~PopupWindow() {
			};

			PopupWindow &operator=(const PopupWindow &w) {
				if(this == &w)
					return *this;
				Window::operator=(w);
				return *this;
			};

			virtual void close(tCoord x,tCoord y) = 0;
		};
	}
}

#endif /* POPUPWINDOW_H_ */
