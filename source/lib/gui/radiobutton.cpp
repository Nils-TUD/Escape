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

#include <sys/common.h>
#include <gui/radiobutton.h>
#include <gui/radiogroup.h>
#include <algorithm>

namespace gui {
	RadioButton::RadioButton(RadioGroup &group,const std::string &text) : Toggle(text), _group(group) {
		_group.add(this);
	}
	RadioButton::~RadioButton() {
		_group.remove(this);
	}

	bool RadioButton::doSetSelected(bool) {
		_group.setSelected(this);
		return true;
	}
	Size RadioButton::getPrefSize() const {
		gsize_t fwidth = getGraphics()->getFont().getStringWidth(getText());
		gsize_t fheight = getGraphics()->getFont().getSize().height;
		return Size(fwidth + TEXT_PADDING + 4 + fheight,
					std::max((gsize_t)(fheight + getTheme().getTextPadding() * 2),fheight));
	}

	void RadioButton::paint(Graphics &g) {
		gsize_t cheight = g.getFont().getSize().height;
		gsize_t height = getSize().height;
		gsize_t circleSize = 4 + cheight;

		g.setColor(getTheme().getColor(Theme::CTRL_BACKGROUND));
		g.fillRect(Pos(0,0),getSize());

		g.setColor(getTheme().getColor(Theme::CTRL_BORDER));
		g.drawCircle(Pos(4 + cheight / 2,height / 2),cheight / 2);

		g.setColor(getTheme().getColor(Theme::TEXT_BACKGROUND));
		g.fillCircle(Pos(4 + cheight / 2,height / 2),cheight / 2 - 1);

		if(isSelected()) {
			g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
			g.fillCircle(Pos(4 + cheight / 2,height / 2),cheight / 2 - 4);
		}

		g.setColor(getTheme().getColor(Theme::CTRL_FOREGROUND));
		g.drawString(circleSize + TEXT_PADDING,(height - cheight) / 2 + 1,getText());
	}
}
