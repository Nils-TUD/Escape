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
		friend class Window;
		friend class Panel;
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
			: _g(NULL), _parent(NULL), _x(x), _y(y), _width(width), _height(height), _mlist(NULL),
			  _klist(NULL), _enableRepaint(true) {
		};
		/**
		 * Copy-constructor
		 *
		 * @param e the ui-element to copy
		 */
		UIElement(const UIElement &e)
			: _g(NULL), _parent(NULL), _x(e._x), _y(e._y), _width(e._width), _height(e._height),
			_mlist(e._mlist), _klist(e._klist), _enableRepaint(e._enableRepaint) {
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
		 * @return the x-position of this element relative to the parent
		 */
		inline gpos_t getX() const {
			return _x;
		};
		/**
		 * @return the x-position in the window
		 */
		gpos_t getWindowX() const;
		/**
		 * @return the x-position on the screen
		 */
		gpos_t getScreenX() const;
		/**
		 * @return the y-position of this element relative to the parent
		 */
		inline gpos_t getY() const {
			return _y;
		};
		/**
		 * @return the y-position in the window
		 */
		gpos_t getWindowY() const;
		/**
		 * @return the y-position on the screen
		 */
		gpos_t getScreenY() const;
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
		 * @return the window this ui-element belongs to
		 */
		Window *getWindow();
		/**
		 * @return the parent-element (may be NULL if not added to a panel yet or its a window)
		 */
		inline UIElement *getParent() {
			return _parent;
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
		 * Resizes the ui-element to width and height
		 *
		 * @param width the new width
		 * @param height the new height
		 */
		virtual void resizeTo(gsize_t width,gsize_t height) = 0;
		/**
		 * Moves the ui-element to x,y.
		 *
		 * @param x the new x-position
		 * @param y the new y-position
		 */
		virtual void moveTo(gpos_t x,gpos_t y) = 0;

		/**
		 * Paints the control
		 *
		 * @param g the graphics-object
		 */
		virtual void paint(Graphics &g) = 0;
		/**
		 * Repaints the control, i.e. calls paint() and requests vesa to update this region
		 */
		virtual void repaint();
		/**
		 * Requests vesa to update the dirty region
		 */
		void requestUpdate();

		/**
		 * @return whether calls of repaint() actually perform a repaint
		 */
		inline bool isRepaintEnabled() const {
			return _enableRepaint;
		};
		/**
		 * Sets whether calls of repaint() actually perform a repaint. That is, by setting it to
		 * false, calls of repaint() are ignored.
		 *
		 * @param en the new value
		 */
		inline void setRepaintEnabled(bool en) {
			_enableRepaint = en;
		};

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

	private:
		/**
		 * Sets the parent of this control (used by Panel)
		 *
		 * @param e the parent
		 */
		virtual void setParent(UIElement *e) {
			_parent = e;
		};

		// only used internally
		void notifyListener(const MouseEvent &e);
		void notifyListener(const KeyEvent &e);

	private:
		Graphics *_g;
		UIElement *_parent;
		gpos_t _x;
		gpos_t _y;
		gsize_t _width;
		gsize_t _height;
		vector<MouseListener*> *_mlist;
		vector<KeyListener*> *_klist;
		bool _enableRepaint;
	};
}

#endif /* UIELEMENT_H_ */
