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

#include <sys/common.h>
#include <gui/wrapper.h>

namespace gui {
	class Border : public Wrapper {
	public:
		enum Location {
			LEFT	= 1,
			TOP		= 2,
			RIGHT	= 4,
			BOTTOM	= 8,
			ALL		= LEFT | TOP | RIGHT | BOTTOM
		};

		Border(std::shared_ptr<Control> ctrl,uint locs = ALL,gsize_t bsize = 1)
			: Wrapper(ctrl), _update(), _locs(locs), _size(bsize) {
		}
		Border(std::shared_ptr<Control> ctrl,const Pos &pos,const Size &size,
			   uint locs = ALL,gsize_t bsize = 1)
			: Wrapper(ctrl,pos,size), _update(), _locs(locs), _size(bsize) {
		}

		virtual Size getUsedSize(const Size &avail) const {
			Size border = getBorderSize();
			return _ctrl->getUsedSize(avail - border) + border;
		}

		virtual bool layout();

		virtual bool resizeTo(const Size &size) {
			bool res = Control::resizeTo(size);
			res |= _ctrl->resizeTo(size - getBorderSize());
			return res;
		}

	protected:
		virtual void paint(Graphics &g);

	private:
		Size getBorderSize() const {
			Size s;
			if(_locs & LEFT)
				s.width += _size;
			if(_locs & TOP)
				s.height += _size;
			if(_locs & RIGHT)
				s.width += _size;
			if(_locs & BOTTOM)
				s.height += _size;
			return s;
		}
		Pos getBorderOff() const {
			Pos pos;
			if(_locs & LEFT)
				pos.x += _size;
			if(_locs & TOP)
				pos.y += _size;
			return pos;
		}
		virtual Size getPrefSize() const {
			return _ctrl->getPreferredSize() + getBorderSize();
		}

		Rectangle _update;
		uint _locs;
		gsize_t _size;
	};
}
