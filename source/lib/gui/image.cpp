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

#include <esc/rawfile.h>
#include <gui/image.h>
#include <sys/common.h>

using namespace std;

namespace gui {

void Image::paint(Graphics &g,const Pos &pos) {
	if(!g.getBuffer()->isReady())
		return;

	Pos rpos(pos.x,pos.y);
	Size rsize = getSize();
	if(!g.validateParams(rpos,rsize))
		return;
	rpos -= Size(pos.x,pos.y);

	_painter->reset(&g,pos);

	_img->paint(rpos.x,rpos.y,rsize.width,rsize.height);

	g.updateMinMax(Pos(pos.x + rpos.x,pos.y + rpos.y));
	g.updateMinMax(Pos(pos.x + rpos.x + rsize.width - 1,pos.y + rpos.y + rsize.height - 1));
}

}
