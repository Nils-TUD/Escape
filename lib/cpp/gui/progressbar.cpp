/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <gui/common.h>
#include <gui/progressbar.h>
#include <gui/control.h>

namespace gui {
	Color ProgressBar::FGCOLOR = Color(0xFF,0xFF,0xFF);
	Color ProgressBar::BGCOLOR = Color(0x80,0x80,0x80);
	Color ProgressBar::BARCOLOR = Color(0,0,0xFF);
	Color ProgressBar::BORDER_COLOR = Color(0x20,0x20,0x20);

	ProgressBar &ProgressBar::operator=(const ProgressBar &b) {
		// ignore self-assignments
		if(this == &b)
			return *this;
		Control::operator=(b);
		_position = b._position;
		_text = b._text;
		return *this;
	}

	void ProgressBar::paint(Graphics &g) {
		// draw border
		g.setColor(BORDER_COLOR);
		g.drawRect(0,0,getWidth(),getHeight());

		// draw bar
		u32 barWidth;
		if(_position == 0)
			barWidth = 0;
		else
			barWidth = (getWidth() - 2) / (100.0f / _position);

		g.setColor(BARCOLOR);
		g.fillRect(1,1,barWidth,getHeight() - 2);

		// draw bg
		g.setColor(BGCOLOR);
		g.fillRect(1 + barWidth,1,getWidth() - barWidth - 2,getHeight() - 2);

		// draw text
		g.setColor(FGCOLOR);
		g.drawString((getWidth() - g.getFont().getStringWidth(_text)) / 2 + 1,
				(getHeight() - g.getFont().getHeight()) / 2 + 1,_text);
	}
}
