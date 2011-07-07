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

#ifndef DESKTOPWIN_H_
#define DESKTOPWIN_H_

#include <esc/common.h>
#include <gui/color.h>
#include <gui/window.h>

class DesktopWin : public gui::Window {
public:
	static const gui::Color BGCOLOR;

public:
	DesktopWin(gsize_t width,gsize_t height) : gui::Window("",0,0,width,height,STYLE_DESKTOP) {
	};
	virtual ~DesktopWin() {
	};

	/* overwrite events */
	virtual void onMouseMoved(const gui::MouseEvent &e);
	virtual void onMouseReleased(const gui::MouseEvent &e);
	virtual void onMousePressed(const gui::MouseEvent &e);
	virtual void onKeyPressed(const gui::KeyEvent &e);
	virtual void onKeyReleased(const gui::KeyEvent &e);

	virtual void paint(gui::Graphics &g);

	// no cloning
private:
	DesktopWin(const DesktopWin &w);
	DesktopWin &operator=(const DesktopWin &w);
};

#endif /* DESKTOPWIN_H_ */
