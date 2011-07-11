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
	class Control;
	class ScrollPane;

	/**
	 * The abstract base class for all UI-elements (windows, controls). It has a position, a size,
	 * a graphics object and has callback-methods for events.
	 */
	class UIElement {
		friend class Window;
		friend class Panel;
		friend class Control;
		friend class ScrollPane;

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
		 * @return the width of the content of this ui-element. for normal elements its simply
		 * 	their width. containers may reduce it if they want to paint other stuff besides
		 * 	their children.
		 */
		virtual gsize_t getContentWidth() const {
			// we have to use the minimum of our size and the parent's because some containers
			// above us might restrict the size, while some won't.
			if(_parent)
				return min(_parent->getContentWidth(),_width);
			return _width;
		};
		/**
		 * @return the height of the content of this ui-element. for normal elements its simply
		 * 	their height. containers may reduce it if they want to paint other stuff besides
		 * 	their children.
		 */
		virtual gsize_t getContentHeight() const {
			if(_parent)
				return min(_parent->getContentHeight(),_height);
			return _height;
		};

		/**
		 * @return the preferred width of this ui-element, e.g. the minimum required width to be
		 * 	able to display the whole content
		 */
		virtual gsize_t getPreferredWidth() const = 0;
		/**
		 * @return the preferred height of this ui-element, e.g. the minimum required height to be
		 * 	able to display the whole content
		 */
		virtual gsize_t getPreferredHeight() const = 0;

		/**
		 * Performs the layout-calculation for this ui-element. This is only used by Window and
		 * Panel.
		 */
		virtual void layout() = 0;

		/**
		 * Paints the control
		 *
		 * @param g the graphics-object
		 */
		virtual void paint(Graphics &g) = 0;
		/**
		 * Repaints the control, i.e. calls paint() and requests an update of this region
		 */
		void repaint();

		/**
		 * Paints the given rectangle of the control
		 *
		 * @param g the graphics-object
		 * @param x the x-position relative to this control of the rectangle
		 * @param y the y-position relative to this control of the rectangle
		 * @param width the width of the rectangle
		 * @param height the height of the rectangle
		 */
		virtual void paintRect(Graphics &g,gpos_t x,gpos_t y,gsize_t width,gsize_t height);
		/**
		 * Repaints the given rectangle of the control, i.e. calls paintRect() and requests an
		 * update of this rectangle.
		 */
		void repaintRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

		/**
		 * Requests an update of the dirty region
		 */
		void requestUpdate();

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

		/**
		 * Sets the parent of this control (used by Panel)
		 *
		 * @param e the parent
		 */
		virtual void setParent(UIElement *e) {
			_parent = e;
		};

	private:
		/**
		 * Informs the ui-element that the child c currently has the focus
		 *
		 * @param c the child-control
		 */
		virtual void setFocus(Control *c);

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
