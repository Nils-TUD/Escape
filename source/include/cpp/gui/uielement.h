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

#ifndef UIELEMENT_H_
#define UIELEMENT_H_

#include <esc/common.h>
#include <esc/messages.h>
#include <gui/graphics.h>
#include <gui/event.h>
#include <gui/mouselistener.h>
#include <gui/keylistener.h>
#include <vector>

namespace gui {
	/**
	 * The abstract base class for all UI-elements (windows, controls). It has a position, a size,
	 * a graphics object and has callback-methods for events.
	 */
	class UIElement {
		// window and control need to access _g
		friend class Window;
		friend class Control;

	public:
		/**
		 * Constructor
		 *
		 * @param x the x-position
		 * @param y the y-position
		 * @param width the width
		 * @param height the height
		 */
		UIElement(gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: _g(NULL), _x(x), _y(y), _width(width), _height(height), _mlist(NULL),
			_klist(NULL) {
		};
		/**
		 * Copy-constructor
		 *
		 * @param e the ui-element to copy
		 */
		UIElement(const UIElement &e)
			: _g(NULL), _x(e._x), _y(e._y), _width(e._width), _height(e._height),
			_mlist(e._mlist), _klist(e._klist) {
		}
		/**
		 * Destructor. Free's the memory
		 */
		virtual ~UIElement() {
			delete _mlist;
			delete _klist;
			delete _g;
		};
		/**
		 * Assignment-operator
		 */
		UIElement &operator=(const UIElement &e);

		/**
		 * @return the x-position of this element
		 */
		inline gpos_t getX() const {
			return _x;
		};
		/**
		 * @return the y-position of this element
		 */
		inline gpos_t getY() const {
			return _y;
		};
		/**
		 * @return the width of this element
		 */
		inline gsize_t getWidth() const {
			return _width;
		};
		/**
		 * @return the height of this element
		 */
		inline gsize_t getHeight() const {
			return _height;
		};

		/**
		 * @return the graphics-object
		 */
		inline Graphics *getGraphics() const {
			return _g;
		};

		/**
		 * Adds the given mouse-listener to this ui-element
		 *
		 * @param l the mouse-listener
		 */
		void addMouseListener(MouseListener *l);
		/**
		 * Removes the given mouse-listener
		 *
		 * @param l the mouse-listener
		 */
		void removeMouseListener(MouseListener *l);

		/**
		 * Adds the given key-listener to this ui-element
		 *
		 * @param l the key-listener
		 */
		void addKeyListener(KeyListener *l);
		/**
		 * Removes the given key-listener
		 *
		 * @param l the key-listener
		 */
		void removeKeyListener(KeyListener *l);

		/**
		 * The callback-methods for the events
		 *
		 * @param e the event
		 */
		virtual void onMouseMoved(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onKeyPressed(const KeyEvent &e);
		virtual void onKeyReleased(const KeyEvent &e);

		/**
		 * Repaints the control, i.e. calls paint() and requests vesa to update this region
		 */
		virtual void repaint();
		/**
		 * Paints the control
		 *
		 * @param g the graphics-object
		 */
		virtual void paint(Graphics &g) = 0;
		/**
		 * Requests vesa to update the dirty region
		 */
		void requestUpdate();

	protected:
		/**
		 * Sets the x-position
		 *
		 * @param x the new position
		 */
		inline void setX(gpos_t x) {
			_x = x;
		};
		/**
		 * Sets the y-position
		 *
		 * @param y the new position
		 */
		inline void setY(gpos_t y) {
			_y = y;
		};
		/**
		 * Sets the width
		 *
		 * @param width the new width
		 */
		inline void setWidth(gsize_t width) {
			_width = width;
		};
		/**
		 * Sets the height
		 *
		 * @param height the new height
		 */
		inline void setHeight(gsize_t height) {
			_height = height;
		};
		/**
		 * @return the id of the window this ui-element belongs to
		 */
		virtual gwinid_t getWindowId() const = 0;

	private:
		// only used internally
		void notifyListener(const MouseEvent &e);
		void notifyListener(const KeyEvent &e);

	private:
		Graphics *_g;
		gpos_t _x;
		gpos_t _y;
		gsize_t _width;
		gsize_t _height;
		vector<MouseListener*> *_mlist;
		vector<KeyListener*> *_klist;
	};
}

#endif /* UIELEMENT_H_ */
