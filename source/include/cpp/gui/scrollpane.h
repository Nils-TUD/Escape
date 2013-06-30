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
#include <gui/control.h>

namespace gui {
	class ScrollPane : public Control {
	private:
		static const gsize_t BAR_SIZE;
		static const gsize_t MIN_SIZE;
		static const gsize_t MIN_BAR_SIZE;
		static const gsize_t SCROLL_FACTOR;

		static const unsigned int FOCUS_CTRL	= 1;
		static const unsigned int FOCUS_HORSB	= 2;
		static const unsigned int FOCUS_VERTSB	= 4;

	public:
		ScrollPane(std::shared_ptr<Control> ctrl)
			: Control(), _ctrl(ctrl), _focus(0), _doingLayout() {
		}
		ScrollPane(std::shared_ptr<Control> ctrl,const Pos &pos,const Size &size)
			: Control(pos,size), _ctrl(ctrl), _focus(0), _doingLayout() {
		}

		virtual Size getPrefSize() const {
			return maxsize(_ctrl->getPreferredSize() + Size(BAR_SIZE,BAR_SIZE),Size(MIN_SIZE,MIN_SIZE));
		}

		virtual Size getUsedSize(const Size &avail) const {
			return maxsize(avail,Size(MIN_SIZE,MIN_SIZE));
		}

		virtual Size getContentSize() const {
			Size size = getSize();
			if(size.width - _ctrl->getPos().x < BAR_SIZE)
				size.width = 0;
			else
				size.width -= _ctrl->getPos().x + BAR_SIZE;
			if(size.height - _ctrl->getPos().y < BAR_SIZE)
				size.height = 0;
			else
				size.height -= _ctrl->getPos().y + BAR_SIZE;
			return size;
		}

		virtual void onMouseMoved(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseWheel(const MouseEvent &e);

		virtual void layout() {
			_doingLayout = true;
			_ctrl->layout();
			_doingLayout = false;
			getParent()->layoutChanged();
		}

		virtual bool isDirty() const {
			return UIElement::isDirty() || _ctrl->isDirty();
		}

		virtual void print(std::ostream &os, bool rec = true, size_t indent = 0) const;

	protected:
		virtual void paint(Graphics &g);
		virtual void paintRect(Graphics &g,const Pos &pos,const Size &size);

		virtual void resizeTo(const Size &size);
		virtual void moveTo(const Pos &pos) {
			Control::moveTo(pos);
			// don't move the control, its position is relative to us. just refresh the paint-region
			_ctrl->setRegion();
		}
		virtual void setRegion() {
			Control::setRegion();
			_ctrl->setRegion();
		}

		virtual void onFocusGained() {
			Control::onFocusGained();
			if(_focus & FOCUS_CTRL)
				_ctrl->onFocusGained();
		}

		virtual Control *getFocus() {
			if(_focus & FOCUS_CTRL)
				return _ctrl->getFocus();
			if(_focus)
				return this;
			return nullptr;
		}
		virtual const Control *getFocus() const {
			if(_focus & FOCUS_CTRL)
				return _ctrl->getFocus();
			if(_focus)
				return this;
			return nullptr;
		}

		virtual void setParent(UIElement *e) {
			Control::setParent(e);
			_ctrl->setParent(this);
		}

		virtual void layoutChanged() {
			if(_doingLayout)
				return;
			layout();
			repaint();
		}

	private:
		void scrollBy(short mx,short my);
		virtual void setFocus(Control *c) {
			if(c)
				_focus = FOCUS_CTRL;
			else
				_focus &= ~FOCUS_CTRL;
			_parent->setFocus(this);
		}

	private:
		void paintBars(Graphics &g);
		gpos_t getBarPos(gsize_t ctrlSize,gsize_t viewable,gpos_t ctrlPos);
		gsize_t getBarSize(gsize_t ctrlSize,gsize_t viewable);

	private:
		std::shared_ptr<Control> _ctrl;
		unsigned int _focus;
		bool _doingLayout;
	};
}
