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
#include <gui/toggle.h>

namespace gui {
	void Toggle::onFocusGained() {
		Control::onFocusGained();
		setFocused(true);
	}
	void Toggle::onFocusLost() {
		Control::onFocusLost();
		setFocused(false);
	}

	void Toggle::onKeyReleased(const KeyEvent &e) {
		uchar keycode = e.getKeyCode();
		UIElement::onKeyReleased(e);
		if(keycode == VK_ENTER || keycode == VK_SPACE) {
			if(doSetSelected(!_selected))
				repaint();
		}
	}
	void Toggle::onMouseReleased(const MouseEvent &e) {
		UIElement::onMouseReleased(e);
		if(doSetSelected(!_selected))
			repaint();
	}
}
