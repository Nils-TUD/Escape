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
#include <gui/application.h>
#include <esc/proc.h>
#include <stdio.h>
#include <iostream>
#include "desktopwin.h"

const gsize_t DesktopWin::PADDING = 10;
const gsize_t DesktopWin::ICON_SIZE = 30;
const Color DesktopWin::BGCOLOR = Color(0xd5,0xe6,0xf3);

void DesktopWin::actionPerformed(UIElement& el) {
	ImageButton *btn = dynamic_cast<ImageButton*>(&el);
	if(btn) {
		map<ImageButton*,Shortcut*>::iterator it = _shortcuts.find(btn);
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
	else {
		for(map<gwinid_t,Button*>::iterator wit = _windows.begin(); wit != _windows.end(); ++wit) {
			if((*wit).second == &el) {
				Application::getInstance()->requestActiveWindow((*wit).first);
				break;
			}
		}
	}
}

void DesktopWin::onWindowCreated(gwinid_t id,const std::string& title) {
	Button *b = new Button(title);
	b->addListener(this);
	_windows[id] = b;
	_winPanel.add(*b);
	layout();
	repaint();
}
void DesktopWin::onWindowActive(gwinid_t id) {
	if(_active)
		_active->setBGColor(Color(0x80,0x80,0x80));
	_active = _windows[id];
	if(_active)
		_active->setBGColor(Color(0xC0,0xC0,0xC0));
}
void DesktopWin::onWindowDestroyed(gwinid_t id) {
	Button *b = _windows[id];
	b->removeListener(this);
	_winPanel.remove(*b);
	if(_active == b)
		_active = NULL;
	delete b;
	_windows.erase(id);
	layout();
	repaint();
}
