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
#include <gui/imagebutton.h>
#include <esc/proc.h>
#include <stdio.h>
#include "desktopwin.h"

const gsize_t DesktopWin::PADDING = 10;
const gsize_t DesktopWin::ICON_SIZE = 30;
const gui::Color DesktopWin::BGCOLOR = gui::Color(0xd5,0xe6,0xf3);

void DesktopWin::actionPerformed(gui::UIElement& el) {
	gui::ImageButton &btn = dynamic_cast<gui::ImageButton&>(el);
	map<gui::ImageButton*,Shortcut*>::iterator it = _shortcuts.find(&btn);
	if(it != _shortcuts.end()) {
		int pid = fork();
		if(pid == 0) {
			const char *args[2] = {NULL,NULL};
			args[0] = it->second->getApp().c_str();
			exec(args[0],args);
		}
		else if(pid < 0)
			printe("[DESKTOP] Unable to create child-process");
	}
}

void DesktopWin::paint(gui::Graphics &g) {
	g.setColor(BGCOLOR);
	g.fillRect(0,0,getWidth(),getHeight());

	map<gui::ImageButton*,Shortcut*>::iterator it;
	for(it = _shortcuts.begin(); it != _shortcuts.end(); ++it) {
		gui::ImageButton *btn = (*it).first;
		btn->paint(*btn->getGraphics());
	}
}
