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

#include <esc/common.h>
#include <gui/splitpanel.h>
#include <gui/window.h>

namespace gui {
	bool SplitPanel::layout() {
		bool res = false;
		_doingLayout = true;
		gsize_t pad = getTheme().getPadding();
		Size total = getSize() - Size(pad * 2,pad * 2);

		// determine position by preferred sizes, if not already done
		if(_position == -1) {
			const std::shared_ptr<Control> &first = _controls[1];
			const std::shared_ptr<Control> &second = _controls[2];
			Size fsize = first.get() ? first.get()->getPreferredSize() : Size();
			if(!second.get())
				_position = 100;
			else if(_orientation == VERTICAL)
				_position = 100.0 * ((double)fsize.width / total.width);
			else
				_position = 100.0 * ((double)fsize.height / total.height);
		}

		Size first;
		if(_orientation == VERTICAL)
			first = Size(total.width * (_position / 100.0),total.height);
		else
			first = Size(total.width,total.height * (_position / 100.0));

		Pos pos(pad,pad);
		if(_controls[1]) {
			res |= _controls[1]->resizeTo(first);
			res |= _controls[1]->moveTo(pos);
			if(_orientation == VERTICAL)
				pos.x += first.width;
			else
				pos.y += first.height;
		}

		Size sepSize = _controls[0]->getPreferredSize();
		res |= _controls[0]->moveTo(pos);
		if(_orientation == VERTICAL) {
			res |= _controls[0]->resizeTo(Size(sepSize.width,total.height));
			pos.x += sepSize.width;
		}
		else {
			res |= _controls[0]->resizeTo(Size(total.width,sepSize.height));
			pos.y += sepSize.height;
		}

		if(_controls[2]) {
			res |= _controls[2]->moveTo(pos);
			res |= _controls[2]->resizeTo(total - Size(pos));
		}

		_doingLayout = false;
		if(res)
			getParent()->layoutChanged();
		return res;
	}

	void SplitPanel::refresh() {
		layout();
		repaint();
		_refreshing = false;
	}

	void SplitPanel::onMouseMoved(const MouseEvent &e) {
		if(_moving) {
			double oldpos = _position;
			if(_orientation == VERTICAL)
				_position = 100.0 * ((double)e.getPos().x / getSize().width);
			else
				_position = 100.0 * ((double)e.getPos().y / getSize().height);
			if(!_refreshing && _position != oldpos) {
				_refreshing = true;
				Application::getInstance()->executeIn(TIMEOUT,std::make_memfun(this,&SplitPanel::refresh));
			}
		}
	}

	void SplitPanel::onMouseReleased(const MouseEvent &e) {
		Panel::onMouseReleased(e);
		if(_moving)
			_moving = false;
	}

	void SplitPanel::onMousePressed(const MouseEvent &e) {
		const std::shared_ptr<Control> &splitter = _controls[0];
		if(e.getPos().x >= splitter->getPos().x &&
			e.getPos().x < (gpos_t)(splitter->getPos().x + splitter->getSize().width) &&
			e.getPos().y >= splitter->getPos().y &&
			e.getPos().y < (gpos_t)(splitter->getPos().y + splitter->getSize().height)) {
			_moving = true;
		}
		else
			Panel::onMousePressed(e);
	}

	bool SplitPanel::resizeTo(const Size &size) {
		gpos_t diffw = getSize().width - size.width;
		gpos_t diffh = getSize().height - size.height;

		Control::resizeTo(size);
		if(diffw || diffh)
			return layout();
		return false;
	}

	Size SplitPanel::combine(Size a,Size b,Size splitter) const {
		gsize_t pad = getTheme().getPadding();
		Size total = a;
		if(_orientation == VERTICAL) {
			total.width += b.width + splitter.width;
			total.height = std::max(total.height,b.height);
		}
		else {
			total.height += b.height + splitter.height;
			total.width = std::max(total.width,b.width);
		}
		return total + Size(pad * 2,pad * 2);
	}

	Size SplitPanel::getPrefSize() const {
		const std::shared_ptr<Control> &first = _controls[1];
		const std::shared_ptr<Control> &second = _controls[2];
		Size fsize = first.get() ? first.get()->getPreferredSize() : Size();
		Size ssize = second.get() ? second.get()->getPreferredSize() : Size();
		return combine(fsize,ssize,_controls[0]->getPreferredSize());
	}

	Size SplitPanel::getUsedSize(const Size &avail) const {
		return avail;
	}
}
