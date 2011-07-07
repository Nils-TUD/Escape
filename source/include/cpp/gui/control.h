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

#ifndef CONTROL_H_
#define CONTROL_H_

#include <esc/common.h>
#include <gui/uielement.h>

namespace gui {
	class Window;

	class Control : public UIElement {
		friend class Window;

	public:
		Control(gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: UIElement(x,y,width,height), _w(NULL) {
		};
		Control(const Control &c)
			: UIElement(c), _w(NULL) {
			// don't assign the window; the user has to do it manually
		};
		virtual ~Control() {

		};
		Control &operator=(const Control &c);

		inline Window &getWindow() const {
			return *_w;
		};

		virtual void onFocusGained();
		virtual void onFocusLost();

	protected:
		gwinid_t getWindowId() const;

	private:
		void setWindow(Window *w);

	private:
		Window *_w;
	};
}

#endif /* CONTROL_H_ */
