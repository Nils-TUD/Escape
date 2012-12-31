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
		static const gsize_t BAR_SIZE			= 20;
		static const gsize_t SCROLL_FACTOR		= 10;

		static const unsigned int FOCUS_CTRL	= 1;
		static const unsigned int FOCUS_HORSB	= 2;
		static const unsigned int FOCUS_VERTSB	= 4;

	public:
		ScrollPane(Control *ctrl)
			: Control(), _ctrl(ctrl), _focus(0) {
		};
		ScrollPane(Control *ctrl,gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: Control(x,y,width,height), _ctrl(ctrl), _focus(0) {
		};
		virtual ~ScrollPane() {
			delete _ctrl;
		};

		virtual gsize_t getMinWidth() const {
			return _ctrl->getMinWidth() + BAR_SIZE;
		};
		virtual gsize_t getMinHeight() const {
			return _ctrl->getMinHeight() + BAR_SIZE;
		};

		virtual gsize_t getContentWidth() const {
			if(_width - _ctrl->getX() < BAR_SIZE)
				return 0;
			return _width - _ctrl->getX() - BAR_SIZE;
		};
		virtual gsize_t getContentHeight() const {
			if(_height - _ctrl->getY() < BAR_SIZE)
				return 0;
			return _height - _ctrl->getY() - BAR_SIZE;
		};

		virtual void onMouseMoved(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseWheel(const MouseEvent &e);

		virtual void layout() {
			_ctrl->layout();
		};

	protected:
		virtual void paint(Graphics &g);
		virtual void paintRect(Graphics &g,gpos_t x,gpos_t y,gsize_t width,gsize_t height);

		virtual void resizeTo(gsize_t width,gsize_t height);
		virtual void moveTo(gpos_t x,gpos_t y) {
			Control::moveTo(x,y);
			// don't move the control, its position is relative to us. just refresh the paint-region
			_ctrl->setRegion();
		};
		virtual void setRegion() {
			Control::setRegion();
			_ctrl->setRegion();
		};

		virtual void onFocusGained() {
			Control::onFocusGained();
			if(_focus & FOCUS_CTRL)
				_ctrl->onFocusGained();
		};

		virtual Control *getFocus() {
			if(_focus & FOCUS_CTRL)
				return _ctrl->getFocus();
			if(_focus)
				return this;
			return NULL;
		};
		virtual const Control *getFocus() const {
			if(_focus & FOCUS_CTRL)
				return _ctrl->getFocus();
			if(_focus)
				return this;
			return NULL;
		};

		virtual void setParent(UIElement *e) {
			Control::setParent(e);
			_ctrl->setParent(this);
		};

	private:
		void scrollBy(short mx,short my);
		virtual void setFocus(Control *c) {
			if(c)
				_focus = FOCUS_CTRL;
			else
				_focus &= ~FOCUS_CTRL;
			_parent->setFocus(this);
		};

	private:
		void paintBars(Graphics &g);
		gpos_t getBarPos(gsize_t ctrlSize,gsize_t viewable,gpos_t ctrlPos);
		gsize_t getBarSize(gsize_t ctrlSize,gsize_t viewable);

	private:
		Control *_ctrl;
		unsigned int _focus;
	};
}
