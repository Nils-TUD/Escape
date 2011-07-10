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
	class Panel;
	class BorderLayout;

	/**
	 * The abstract base class for all controls
	 */
	class Control : public UIElement {
		friend class Panel;
		friend class BorderLayout;

	public:
		/**
		 * Constructor
		 *
		 * @param x the x-position
		 * @param y the y-position
		 * @param width the width
		 * @param height the height
		 */
		Control(gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: UIElement(x,y,width,height) {
		};
		/**
		 * Copy-constructor
		 *
		 * @param c the control to copy
		 */
		Control(const Control &c)
			: UIElement(c) {
		};
		/**
		 * Destructor
		 */
		virtual ~Control() {
		};
		/**
		 * Assignment operator
		 */
		Control &operator=(const Control &c);

		/**
		 * Is called as soon as this control received the focus
		 */
		virtual void onFocusGained();
		/**
		 * Is called as soon as this control lost the focus
		 */
		virtual void onFocusLost();

	protected:
		/**
		 * Resizes the ui-element to width and height
		 *
		 * @param width the new width
		 * @param height the new height
		 */
		virtual void resizeTo(gsize_t width,gsize_t height);
		/**
		 * Moves the ui-element to x,y.
		 *
		 * @param x the new x-position
		 * @param y the new y-position
		 */
		virtual void moveTo(gpos_t x,gpos_t y);

	private:
		/**
		 * Sets the parent of this control (used by Panel)
		 *
		 * @param e the parent
		 */
		virtual void setParent(UIElement *e);

		/**
		 * @return the control that has the focus (not a panel!) or NULL if no one
		 */
		virtual Control *getFocus() {
			// panel returns the focused control on the panel; a control does already return itself
			return this;
		};
		virtual const Control *getFocus() const {
			return this;
		};
	};
}

#endif /* CONTROL_H_ */
