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
	Size ProgressBar::getPrefSize() const {
		gsize_t pad = getTheme().getTextPadding();
		const Font &f = getGraphics()->getFont();
		return Size(f.getStringWidth(_text) + pad * 2,f.getSize().height + pad * 2);
	}

	void ProgressBar::paint(Graphics &g) {
		Size size = getSize();
		// draw border
		g.setColor(getTheme().getColor(Theme::CTRL_BORDER));
		g.drawRect(Pos(0,0),size);

		// draw bar
		gsize_t barWidth;
		if(_position == 0)
			barWidth = 0;
		else
			barWidth = (size.width - 2) / (100.0f / _position);

		g.setColor(getTheme().getColor(Theme::SEL_BACKGROUND));
		g.fillRect(1,1,barWidth,size.height - 2);

		// draw bg
		g.setColor(getTheme().getColor(Theme::CTRL_LIGHTBACK));
		g.fillRect(1 + barWidth,1,size.width - barWidth - 2,size.height - 2);

		// draw text
		g.setColor(getTheme().getColor(Theme::SEL_FOREGROUND));
		gsize_t count = g.getFont().limitStringTo(_text,size.width);
		if(count > 0) {
			g.drawString((size.width - g.getFont().getStringWidth(_text.substr(0,count))) / 2 + 1,
					(size.height - g.getFont().getSize().height) / 2 + 1,_text,0,count);
		}
	}
}
