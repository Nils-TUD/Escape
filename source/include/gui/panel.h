/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <gui/event/event.h>
#include <gui/layout/layout.h>
#include <gui/control.h>
#include <sys/common.h>
#include <vector>

namespace gui {
	class Window;
	class ScrollPane;

	/**
	 * A panel is a container for control-elements
	 */
	class Panel : public Control {
		friend class Window;
		friend class Control;
		friend class ScrollPane;

	public:
		typedef std::vector<std::shared_ptr<Control>>::iterator iterator;

		/**
		 * Creates an empty panel with given layout.
		 *
		 * @param l the layout (may be empty)
		 */
		Panel(std::shared_ptr<Layout> l = std::shared_ptr<Layout>())
			: Control(), _focus(nullptr), _controls(), _layout(l), _doingLayout() {
		}
		/**
		 * Creates an empty panel at given position, with given size and with given layout.
		 *
		 * @param pos the position
		 * @param size the size
		 * @param l the layout (may be empty)
		 */
		Panel(const Pos &pos,const Size &size,std::shared_ptr<Layout> l = std::shared_ptr<Layout>())
			: Control(pos,size), _focus(nullptr), _controls(), _layout(l), _doingLayout() {
		}

		virtual Rectangle getVisibleRect(const Rectangle &rect) const {
			if(rect.empty())
				return rect;
			return _parent->getVisibleRect(intersection(rect,Rectangle(getWindowPos(),getSize())));
		}

		virtual Size getUsedSize(const Size &avail) const {
			if(_layout) {
				gsize_t pad = getTheme().getPadding();
				Size padsize = Size(pad * 2,pad * 2);
				return _layout->getUsedSize(subsize(avail,padsize)) + padsize;
			}
			return UIElement::getUsedSize(avail);
		}

		/**
		 * @return the layout
		 */
		std::shared_ptr<Layout> getLayout() const {
			return _layout;
		}
		/**
		 * Sets the used layout for this panel. This is only possible if no controls have been
		 * added yet.
		 *
		 * @param l the layout
		 */
		void setLayout(std::shared_ptr<Layout> l) {
			if(_controls.size() > 0) {
				throw std::logic_error("This panel does already have controls;"
						" you can't change the layout afterwards");
			}
			_layout = l;
		}

		/**
		 * Performs a layout-calculation for this panel and all childs
		 */
		virtual bool layout();

		/**
		 * The event-callbacks
		 *
		 * @param e the event
		 */
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseWheel(const MouseEvent &e);

		virtual bool resizeTo(const Size &size);
		virtual bool moveTo(const Pos &pos);

		/**
		 * Adds the given control to this panel.
		 *
		 * @param c the control
		 * @param pos the position-specification for the layout
		 */
		void add(std::shared_ptr<Control> c,Layout::pos_type pos = 0);

		/**
		 * @param idx the index (not the position in the layout!) of the control
		 * @return the control at index <idx>
		 */
		std::shared_ptr<Control> get(size_t idx) {
			return _controls[idx];
		}

		/**
		 * @return the beginning of the control list
		 */
		iterator begin() {
			return _controls.begin();
		}
		/**
		 * @return the end of the control list
		 */
		iterator end() {
			return _controls.end();
		}

		/**
		 * Removes the given control from this panel.
		 *
		 * @param c the control
		 * @param pos the position-specification for the layout
		 */
		void remove(std::shared_ptr<Control> c,Layout::pos_type pos = 0);

		/**
		 * Removes all controls from this panel.
		 */
		void removeAll();

		virtual bool isDirty() const {
			for(auto it = _controls.begin(); it != _controls.end(); ++it) {
				if((*it)->isDirty())
					return true;
			}
			return UIElement::isDirty();
		}

		virtual void print(esc::OStream &os, bool rec = true, size_t indent = 0) const;

	protected:
		virtual void paintBackground(Graphics &g);
		virtual void paint(Graphics &g);

		virtual void setRegion();

		/**
		 * @return the control that has the focus (not a panel!) or nullptr if no one
		 */
		virtual Control *getFocus() {
			if(_focus)
				return _focus->getFocus();
			return nullptr;
		}

		virtual void layoutChanged();

	private:
		void passToCtrl(const MouseEvent &e,bool focus);

		virtual Size getPrefSize() const {
			if(_layout) {
				gsize_t pad = getTheme().getPadding();
				return _layout->getPreferredSize() + Size(pad * 2,pad * 2);
			}
			return Size();
		}
		virtual void setParent(UIElement *e) {
			Control::setParent(e);
			for(auto it = _controls.begin(); it != _controls.end(); ++it)
				(*it)->setParent(this);
		}
		virtual void setFocus(Control *c) {
			_focus = c;
			if(_parent)
				_parent->setFocus(c ? this : nullptr);
		}

	protected:
		Control *_focus;
		std::vector<std::shared_ptr<Control>> _controls;
		std::shared_ptr<Layout> _layout;
		bool _ctrlsChanged;
		bool _doingLayout;
	};
}
