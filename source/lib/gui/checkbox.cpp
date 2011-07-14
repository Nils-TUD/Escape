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
#include <gui/checkbox.h>
#include <gui/control.h>

namespace gui {
	const gsize_t Checkbox::CROSS_SIZE = 24;

	Checkbox &Checkbox::operator=(const Checkbox &b) {
		// ignore self-assignments
		if(this == &b)
			return *this;
		Control::operator=(b);
		_focused = false;
		_checked = b._checked;
		_text = b._text;
		return *this;
	}

	gsize_t Checkbox::getMinWidth() const {
		return getGraphics()->getFont().getStringWidth(_text) +
				TEXT_PADDING + CROSS_PADDING + CROSS_SIZE;
	}
	gsize_t Checkbox::getMinHeight() const {
		gsize_t fheight = getGraphics()->getFont().getHeight();
		return max((gsize_t)(fheight + getTheme().getTextPadding() * 2),CROSS_SIZE);
	}

	void Checkbox::onFocusGained() {
		Control::onFocusGained();
		_focused = true;
		repaint();
	}
	void Checkbox::onFocusLost() {
		Control::onFocusLost();
		_focused = false;
		repaint();
	}

	void Checkbox::onKeyReleased(const KeyEvent &e) {
		uchar keycode = e.getKeyCode();
		UIElement::onKeyReleased(e);
		if(keycode == VK_ENTER || keycode == VK_SPACE)
			setChecked(!_checked);
	}

	void Checkbox::onMouseReleased(const MouseEvent &e) {
		UIElement::onMouseReleased(e);
		setChecked(!_checked);
	}

	void Checkbox::setChecked(bool checked) {
		_checked = checked;
		repaint();
	}

	void Checkbox::paint(Graphics &g) {
		gsize_t cheight = g.getFont().getHeight();
		gsize_t boxSize = getHeight();

		g.setColor(getTheme().getColor(Theme::CTRL_BACKGROUND));
		g.fillRect(0,0,getWidth(),getHeight());

		g.setColor(getTheme().getColor(Theme::TEXT_BACKGROUND));
		g.fillRect(1,1,boxSize - 2,boxSize - 2);

		g.setColor(getTheme().getColor(Theme::CTRL_LIGHTBORDER));
		g.drawLine(boxSize - 1,0,boxSize - 1,boxSize - 1);
		g.drawLine(0,boxSize - 1,boxSize - 1,boxSize - 1);
		g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
		g.drawLine(0,0,boxSize - 1,0);
		g.drawLine(0,0,0,boxSize - 1);

		if(_checked) {
			g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
			g.drawLine(CROSS_PADDING,CROSS_PADDING,
					boxSize - CROSS_PADDING,boxSize - CROSS_PADDING);
			g.drawLine(boxSize - CROSS_PADDING,CROSS_PADDING,
					CROSS_PADDING,boxSize - CROSS_PADDING);
			g.setColor(getTheme().getColor(Theme::CTRL_LIGHTBORDER));
			g.drawLine(CROSS_PADDING,CROSS_PADDING + 1,
					boxSize - CROSS_PADDING,boxSize - CROSS_PADDING + 1);
			g.drawLine(boxSize - CROSS_PADDING,CROSS_PADDING + 1,
					CROSS_PADDING,boxSize - CROSS_PADDING + 1);
		}

		g.setColor(getTheme().getColor(Theme::CTRL_FOREGROUND));
		g.drawString(boxSize + TEXT_PADDING,(boxSize - cheight) / 2 + 1,_text);
	}
}
