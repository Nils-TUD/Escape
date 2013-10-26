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
#include <gui/border.h>
#include <gui/panel.h>
#include <iomanip>

using namespace std;

namespace gui {
	bool Border::layout() {
		bool res = false;
		_doingLayout = true;
		Pos pos = getBorderOff();
		res |= _ctrl->moveTo(pos);
		res |= _ctrl->resizeTo(getSize() - getBorderSize());
		_doingLayout = false;
		if(res)
			getParent()->layoutChanged();
		return res;
	}

	void Border::paint(Graphics &g) {
		Rectangle prect = g.getPaintRect();
		Pos cstart = prect.getPos() - getBorderOff();
		Size csize = minsize(_ctrl->getSize() - Size(cstart),prect.getSize());
		_ctrl->makeDirty(true);
		_ctrl->repaintRect(cstart,csize,false);

		Size size = getSize();
		g.setColor(getTheme().getColor(Theme::CTRL_BORDER));
		if(_locs & LEFT)
			g.fillRect(Pos(0,0),Size(_size,size.height));
		if(_locs & TOP)
			g.fillRect(Pos(_size,0),Size(size.width - _size,_size));
		if(_locs & RIGHT)
			g.fillRect(Pos(size.width - _size,0),Size(_size,size.height));
		if(_locs & BOTTOM)
			g.fillRect(Pos(_size,size.height - _size),Size(size.width - _size,_size));
	}
}
