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

#include <esc/common.h>
#include <gui/checkbox.h>
#include <gui/control.h>

using namespace std;

namespace gui {
	Size Checkbox::getPrefSize() const {
		gsize_t fwidth = getGraphics()->getFont().getStringWidth(getText());
		gsize_t fheight = getGraphics()->getFont().getSize().height;
		return Size(fwidth + TEXT_PADDING + CROSS_PADDING + fheight,
					max((gsize_t)(fheight + getTheme().getTextPadding() * 2),fheight));
	}

	void Checkbox::paint(Graphics &g) {
		gsize_t cheight = g.getFont().getSize().height;
		gsize_t height = getSize().height;
		gsize_t boxSize = cheight;
		gpos_t boxy = (height - boxSize) / 2;

		g.setColor(getTheme().getColor(Theme::CTRL_BACKGROUND));
		g.fillRect(Pos(0,0),getSize());

		g.setColor(getTheme().getColor(Theme::TEXT_BACKGROUND));
		g.fillRect(1,boxy,boxSize - 1,boxSize - 1);

		g.setColor(getTheme().getColor(Theme::CTRL_LIGHTBORDER));
		g.drawLine(boxSize,boxy,boxSize,boxy + boxSize);
		g.drawLine(0,boxy + boxSize,boxSize,boxy + boxSize);
		g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
		g.drawLine(0,boxy,boxSize,boxy);
		g.drawLine(0,boxy,0,boxy + boxSize);

		if(isSelected()) {
			g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
			g.drawLine(3,boxy + boxSize / 2,boxSize / 2,boxy + boxSize - 3);
			g.drawLine(4,boxy + boxSize / 2,boxSize / 2 + 1,boxy + boxSize - 3);
			g.drawLine(5,boxy + boxSize / 2,boxSize / 2 + 1,boxy + boxSize - 3);
			g.drawLine(boxSize / 2,boxy + boxSize - 3,boxSize - 2,boxy + 2);
			g.setColor(getTheme().getColor(Theme::CTRL_LIGHTBORDER));
			g.drawLine(6,boxy + boxSize / 2,boxSize / 2 + 1,boxy + boxSize - 3);
			g.drawLine(boxSize / 2,boxy + boxSize - 2,boxSize - 2,boxy + 2);
		}

		g.setColor(getTheme().getColor(Theme::CTRL_FOREGROUND));
		g.drawString(boxSize + TEXT_PADDING,(height - cheight) / 2 + 1,getText());
	}
}
