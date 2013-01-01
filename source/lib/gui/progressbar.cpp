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
#include <gui/progressbar.h>
#include <gui/control.h>

namespace gui {
	gsize_t ProgressBar::getPrefWidth() const {
		return getGraphics()->getFont().getStringWidth(_text) + getTheme().getTextPadding() * 2;
	}
	gsize_t ProgressBar::getPrefHeight() const {
		return getGraphics()->getFont().getHeight() + getTheme().getTextPadding() * 2;
	}

	void ProgressBar::paint(Graphics &g) {
		// draw border
		g.setColor(getTheme().getColor(Theme::CTRL_BORDER));
		g.drawRect(0,0,getWidth(),getHeight());

		// draw bar
		gsize_t barWidth;
		if(_position == 0)
			barWidth = 0;
		else
			barWidth = (getWidth() - 2) / (100.0f / _position);

		g.setColor(getTheme().getColor(Theme::SEL_BACKGROUND));
		g.fillRect(1,1,barWidth,getHeight() - 2);

		// draw bg
		g.setColor(getTheme().getColor(Theme::CTRL_LIGHTBACK));
		g.fillRect(1 + barWidth,1,getWidth() - barWidth - 2,getHeight() - 2);

		// draw text
		g.setColor(getTheme().getColor(Theme::SEL_FOREGROUND));
		gsize_t count = g.getFont().limitStringTo(_text,getWidth());
		if(count > 0) {
			g.drawString((getWidth() - g.getFont().getStringWidth(_text.substr(0,count))) / 2 + 1,
					(getHeight() - g.getFont().getHeight()) / 2 + 1,_text,0,count);
		}
	}
}
