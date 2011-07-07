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
#include "desktopwin.h"

const gui::Color DesktopWin::BGCOLOR = gui::Color(0xd5,0xe6,0xf3);

void DesktopWin::onMouseMoved(const gui::MouseEvent &e) {
	UNUSED(e);
}
void DesktopWin::onMouseReleased(const gui::MouseEvent &e) {
	UNUSED(e);
}
void DesktopWin::onMousePressed(const gui::MouseEvent &e) {
	UNUSED(e);
}
void DesktopWin::onKeyPressed(const gui::KeyEvent &e) {
	UNUSED(e);
}
void DesktopWin::onKeyReleased(const gui::KeyEvent &e) {
	UNUSED(e);
}

void DesktopWin::paint(gui::Graphics &g) {
	g.setColor(BGCOLOR);
	g.fillRect(0,0,getWidth(),getHeight());
}
