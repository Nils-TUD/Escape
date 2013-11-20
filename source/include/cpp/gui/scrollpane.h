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
#include <gui/wrapper.h>

namespace gui {
	class ScrollPane : public Wrapper {
		static const unsigned int FOCUS_CTRL	= 1;
		static const unsigned int FOCUS_HORSB	= 2;
		static const unsigned int FOCUS_VERTSB	= 4;

		static const uint SCROLL_TIMEOUT		= 20;

		static const gsize_t SCROLL_FACTOR;
		static const gsize_t MIN_BAR_SIZE;

		struct Movement {
			int x;
			int y;
		};

	public:
		static const gsize_t BAR_SIZE;
		static const gsize_t MIN_SIZE;

		ScrollPane(std::shared_ptr<Control> ctrl)
			: Wrapper(ctrl), _update(), _focus(0), _move() {
		}
		ScrollPane(std::shared_ptr<Control> ctrl,const Pos &pos,const Size &size)
			: Wrapper(ctrl,pos,size), _update(), _focus(0), _move() {
		}

		virtual Rectangle getVisibleRect(const Rectangle &rect) const {
			if(rect.empty())
				return rect;
			Size visible = getSize() - Size(BAR_SIZE,BAR_SIZE);
			return intersection(rect,Rectangle(getWindowPos(),visible));
		}

		virtual Size getUsedSize(const Size &avail) const {
			return maxsize(avail,Size(MIN_SIZE,MIN_SIZE));
		}

		virtual void onMouseMoved(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onMouseWheel(const MouseEvent &e);

		/**
		 * Scrolls horizontally by <x> pixels and vertically by <y>.
		 *
		 * @param x the amount to scroll horizontally (negative for left, positive for right)
		 * @param y the amount to scroll vertically (negative for up, positive for down)
		 * @param update whether to request an update
		 */
		void scrollBy(int x,int y,bool update = true);
		/**
		 * Scrolls up/down by the given amount of pages
		 *
		 * @param x the pages in x-direction (negative for left, positive for right)
		 * @param y the pages in y-direction (negative for up, positive for down)
		 * @param update whether to request an update
		 */
		void scrollPages(int x,int y,bool update = true) {
			scrollBy(x * SCROLL_FACTOR,y * SCROLL_FACTOR,update);
		}

		/**
		 * Scrolls to the left
		 *
		 * @param update whether to request an update
		 */
		void scrollToTop(bool update = true) {
			scrollBy(0,std::min(0,_ctrl->getPos().y),update);
		}
		/**
		 * Scrolls to the bottom
		 *
		 * @param update whether to request an update
		 */
		void scrollToBottom(bool update = true) {
			Size visible = getVisible();
			Size max = visible - _ctrl->getSize();
			scrollBy(0,_ctrl->getPos().y - max.height,update);
		}

		virtual bool layout() {
			_doingLayout = true;
			bool res = _ctrl->layout();
			_doingLayout = false;
			getParent()->layoutChanged();
			return res;
		}

		virtual bool resizeTo(const Size &size);

	protected:
		virtual void paint(Graphics &g);
		virtual void paintRect(Graphics &g,const Pos &pos,const Size &size);

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

		virtual void layoutChanged() {
			if(_doingLayout)
				return;
			layout();
			repaint();
		}

	private:
		void performScroll() {
			scrollRelatively(_move.x,_move.y);
			_move.x = _move.y = 0;
		}

		virtual Size getPrefSize() const {
			return maxsize(_ctrl->getPreferredSize() + Size(BAR_SIZE,BAR_SIZE),Size(MIN_SIZE,MIN_SIZE));
		}
		Size getVisible() {
			return Size(std::max<gsize_t>(0,getSize().width - BAR_SIZE),
					std::max<gsize_t>(0,getSize().height - BAR_SIZE));
		}
		void scrollRelatively(int x,int y) {
			Size visible = getVisible();
			x = (int)(_ctrl->getSize().width / ((double)visible.width / x));
			y = (int)(_ctrl->getSize().height / ((double)visible.height / y));
			scrollBy(x,y);
		}
		virtual void setFocus(Control *c) {
			if(c)
				_focus = FOCUS_CTRL;
			else
				_focus &= ~FOCUS_CTRL;
			Wrapper::setFocus(c);
		}

	private:
		void paintBars(Graphics &g);
		gpos_t getBarPos(gsize_t ctrlSize,gsize_t viewable,gpos_t ctrlPos);
		gsize_t getBarSize(gsize_t ctrlSize,gsize_t viewable);

	private:
		Rectangle _update;
		unsigned int _focus;
		Movement _move;
	};
}
