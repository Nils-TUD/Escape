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

#pragma once

#include <esc/common.h>
#include <esc/messages.h>
#include <gui/graphics/graphics.h>
#include <gui/event/event.h>
#include <gui/event/subscriber.h>
#include <gui/application.h>
#include <gui/theme.h>
#include <memory>
#include <vector>

namespace gui {
	class Control;
	class ScrollPane;
	class Layout;

	template<typename T,typename... Args>
	inline std::shared_ptr<T> make_control(Args&&... args) {
		return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
	}

	/**
	 * The abstract base class for all UI-elements (windows, controls). It has a position, a size,
	 * a graphics object and has callback-methods for events.
	 */
	class UIElement {
		friend class Window;
		friend class Panel;
		friend class Control;
		friend class ScrollPane;
		friend class Layout;

	public:
		typedef unsigned id_type;
		typedef Sender<UIElement&,const MouseEvent&> mouseev_type;
		typedef Sender<UIElement&,const KeyEvent&> keyev_type;

	protected:
		/**
		 * Creates an ui-element at position 0,0 and 0x0 pixels large. When using a layout, this
		 * will determine the actual position and size.
		 */
		UIElement()
			: _id(_nextid++), _g(nullptr), _parent(nullptr),
			  _theme(Application::getInstance()->getDefaultTheme()),
			  _pos(), _size(), _prefSize(), _mouseMoved(), _mousePressed(), _mouseReleased(),
			  _mouseWheel(), _keyPressed(), _keyReleased(), _dirty(true) {
		}
		/**
		 * Constructor that specifies a position and size explicitly. This can be used if no layout
		 * is used or if a different preferred size than the min size is desired.
		 *
		 * @param x the x-position
		 * @param y the y-position
		 * @param width the width
		 * @param height the height
		 */
		UIElement(const Pos &pos,const Size &size)
			: _id(_nextid++), _g(nullptr), _parent(nullptr),
			  _theme(Application::getInstance()->getDefaultTheme()),
			  _pos(pos), _size(size), _prefSize(size), _mouseMoved(), _mousePressed(),
			  _mouseReleased(), _mouseWheel(), _keyPressed(), _keyReleased(), _dirty(true) {
		}

	public:
		/**
		 * Destructor. Free's the memory
		 */
		virtual ~UIElement() {
			if(_g)
				_g->detach(this);
			delete _g;
		}

		/**
		 * @return the id of this UIElement (unique in one application)
		 */
		id_type getId() const {
			return _id;
		}

		/**
		 * @return the position in the window
		 */
		Pos getWindowPos() const;
		/**
		 * @return the position on the screen
		 */
		Pos getScreenPos() const;
		/**
		 * @return the position of this element relative to the parent
		 */
		Pos getPos() const {
			return _pos;
		}
		/**
		 * @return the size of this element
		 */
		Size getSize() const {
			return _size;
		}

		/**
		 * Calculates the part of <rect> that is visible according to the parent controls.
		 *
		 * @param rect the rectangle
		 * @return the visible part of it
		 */
		virtual Rectangle getVisibleRect(const Rectangle &rect) const {
			if(_parent) {
				Rectangle r = _parent->getVisibleRect(rect);
				return Rectangle(minpos(r.getPos(),rect.getPos()),minsize(r.getSize(),rect.getSize()));
			}
			return rect;
		}

		/**
		 * @return the preferred size of this ui-element
		 */
		Size getPreferredSize() const {
			if(_prefSize.width && _prefSize.height)
				return _prefSize;
			Size pref = getPrefSize();
			return Size(_prefSize.width ? _prefSize.width : pref.width,
						_prefSize.height ? _prefSize.height : pref.height);
		}

		/**
		 * @return the theme of this ui-element
		 */
		Theme &getTheme() {
			return _theme;
		}
		const Theme &getTheme() const {
			return _theme;
		}

		/**
		 * Performs the layout-calculation for this ui-element. This is only used by Window and
		 * Panel.
		 *
		 * @return true if something has changed
		 */
		virtual bool layout() = 0;

		/**
		 * Repaints the control, i.e. calls paint()
		 *
		 * @param update whether to request an update of the painted region
		 */
		void repaint(bool update = true);

		/**
		 * Repaints the given rectangle of the control, i.e. calls paintRect()
		 *
		 * @param pos the position relative to this control
		 * @param size the size of the rectangle
		 * @param update whether to request an update of the painted region
		 */
		void repaintRect(const Pos &pos,const Size &size,bool update = true);

		/**
		 * Requests an update of the dirty region
		 */
		void requestUpdate();

		/**
		 * @return the graphics-object
		 */
		Graphics *getGraphics() const {
			return _g;
		}

		/**
		 * @return the window this ui-element belongs to
		 */
		Window *getWindow();
		/**
		 * @return the parent-element (may be nullptr if not added to a panel yet or its a window)
		 */
		UIElement *getParent() const {
			return _parent;
		}

		/**
		 * The event senders
		 */
		mouseev_type &mouseMoved() {
			return _mouseMoved;
		}
		mouseev_type &mousePressed() {
			return _mousePressed;
		}
		mouseev_type &mouseReleased() {
			return _mouseReleased;
		}
		mouseev_type &mouseWheel() {
			return _mouseWheel;
		}
		keyev_type &keyPressed() {
			return _keyPressed;
		}
		keyev_type &keyReleased() {
			return _keyReleased;
		}

		/**
		 * The callback-methods for the events
		 *
		 * @param e the event
		 */
		virtual void onMouseMoved(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseWheel(const MouseEvent &e);
		virtual void onKeyPressed(const KeyEvent &e);
		virtual void onKeyReleased(const KeyEvent &e);

		/**
		 * @return whether this UIElement needs a repaint
		 */
		virtual bool isDirty() const {
			return _dirty || _theme.isDirty();
		}
		/**
		 * Sets that this UIElement needs a repaint. Note that you can't make it undirty.
		 *
		 * @param dirty the value
		 */
		void makeDirty(bool dirty) {
			_dirty |= dirty;
		}

		/**
		 * Prints this UI element to <os>, starting with given indent.
		 *
		 * @param os the ostream
		 * @param rec whether to print recursively
		 * @param indent the indent
		 */
		virtual void print(std::ostream &os, bool rec = true, size_t indent = 0) const;

	protected:
		/**
		 * Does the actual painting and has to be implemented by subclasses.
		 *
		 * @param g the graphics object
		 */
		virtual void paint(Graphics &g) = 0;

		/**
		 * Paints the given rectangle of the control
		 *
		 * @param g the graphics-object
		 * @param pos the position relative to this control
		 * @param size the size of the rectangle
		 */
		virtual void paintRect(Graphics &g,const Pos &pos,const Size &size);

		/**
		 * Sets the parent of this control (used by Panel)
		 *
		 * @param e the parent
		 */
		virtual void setParent(UIElement *e) {
			_parent = e;
		}

	private:
		// I've decided to make all ui-elements not-clonable for the following reasons:
		// 1. I can't think of a real reason why cloning is necessary. Of course, one could argue
		//    that its sometimes convenient if objects are similar, but is there really a situtation
		//    in which I can't live without it?
		// 2. It makes it more complicated and error-prone.
		// 3. It introduces the problem that objects of various sub-classes need to be clonable
		//    without knowing their real type. That is, e.g. panels that manage a list of controls
		//    would need to clone all these controls, but don't know their real-type. Therefore we
		//    would need an additional method like "virtual UIElement *clone() const", that needs
		//    to be overwritten by every subclass.
		// 4. Sometimes its not clear what cloning an ui-element should actually mean. For example,
		//    lets say I have a panel with a borderlayout that has a button on it at any position.
		//    Now I'm cloning the button. What should happen? Should the clone be put in the same
		//    slot in the borderlayout? (and overwrite the original) Or be put in another slot? What
		//    if all slots are in use? -> There is no clear and intuitive way to clone this button.
		// So, all in all, it doesn't help much and is not required, but would bring a lot of
		// trouble. Means, better don't allow it at all :)
		UIElement(const UIElement &e);
		UIElement &operator=(const UIElement &e);

		/**
		 * Will be called by a child if it's layout has changed
		 */
		virtual void layoutChanged() {
		}

		/**
		 * Helper for getPreferredSize(). Has to be implemented by subclasses!
		 *
		 * @return the preferred size of this ui-element
		 */
		virtual Size getPrefSize() const = 0;

		/**
		 * Determines what size would be used if <avail> was available. By default, this
		 * is always the maximum of getPreferredSize() and <avail>.
		 *
		 * @param avail the available size
		 * @return the size that would be used
		 */
		virtual Size getUsedSize(const Size &avail) const {
			return maxsize(getPreferredSize(),avail);
		}

		/**
		 * Sets the position
		 *
		 * @param pos the new position
		 */
		void setPos(const Pos &pos) {
			_pos = pos;
		}
		/**
		 * Sets the size
		 *
		 * @param size the new size
		 */
		void setSize(const Size &size) {
			_size = size;
		}

		/**
		 * Informs the ui-element that the child c currently has the focus
		 *
		 * @param c the child-control
		 */
		virtual void setFocus(Control *c);

		/**
		 * Adds some debugging info after drawing an UIElement
		 */
		void debug();

		void makeClean();

	private:
		id_type _id;
		Graphics *_g;
		UIElement *_parent;
		Theme _theme;
		Pos _pos;
		Size _size;
		Size _prefSize;
		mouseev_type _mouseMoved;
		mouseev_type _mousePressed;
		mouseev_type _mouseReleased;
		mouseev_type _mouseWheel;
		keyev_type _keyPressed;
		keyev_type _keyReleased;
		bool _dirty;
		static id_type _nextid;
	};

	static inline std::ostream &operator<<(std::ostream &os,const UIElement &ui) {
		ui.print(os);
		return os;
	}
}
