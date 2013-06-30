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

using namespace gui;
using namespace std;

const gsize_t DesktopWin::PADDING = 10;
const gsize_t DesktopWin::ICON_SIZE = 30;
const Color DesktopWin::BGCOLOR = Color(0xd5,0xe6,0xf3);
const Color DesktopWin::ACTIVE_COLOR = Color(0x90,0x90,0x90);
const gsize_t DesktopWin::TASKBAR_HEIGHT = 24;

DesktopWin::DesktopWin(const Size &size)
	: Window(Pos(0,0),size,DESKTOP),
	  _winPanel(make_control<Panel>(Pos(0,0),Size(0,TASKBAR_HEIGHT),
			  	make_layout<FlowLayout>(FlowLayout::FRONT,FlowLayout::HORIZONTAL,4))),
	  _iconPanel(make_control<Panel>()), _active(nullptr), _windows(), _shortcuts() {
	shared_ptr<Panel> root = getRootPanel();
	root->setLayout(make_layout<BorderLayout>());
	root->getTheme().setPadding(0);
	root->add(_winPanel,BorderLayout::SOUTH);
	root->add(_iconPanel,BorderLayout::CENTER);

	_iconPanel->getTheme().setColor(Theme::CTRL_BACKGROUND,BGCOLOR);

	Application *app = Application::getInstance();
	app->created().subscribe(mem_recv(this,&DesktopWin::onWindowCreated));
	app->activated().subscribe(mem_recv(this,&DesktopWin::onWindowActive));
	app->destroyed().subscribe(mem_recv(this,&DesktopWin::onWindowDestroyed));
}

void DesktopWin::onIconClick(UIElement& el) {
	ImageButton *btn = dynamic_cast<ImageButton*>(&el);
	if(btn) {
		auto it = std::find_if(_shortcuts.begin(),_shortcuts.end(),
			[btn] (const pair<std::shared_ptr<gui::ImageButton>,Shortcut*> &pair) {
				return btn == pair.first.get();
			}
		);
		if(it != _shortcuts.end()) {
			int pid = fork();
			if(pid == 0) {
				const char *args[2] = {nullptr,nullptr};
				args[0] = it->second->getApp().c_str();
				exec(args[0],args);
			}
			else if(pid < 0)
				printe("[DESKTOP] Unable to create child-process");
		}
	}
	else {
		for(auto wit = _windows.begin(); wit != _windows.end(); ++wit) {
			if((*wit).second.get() == &el) {
				Application::getInstance()->requestActiveWindow((*wit).first);
				break;
			}
		}
	}
}

void DesktopWin::onWindowCreated(gwinid_t wid,const string& title) {
	shared_ptr<WinButton> b = make_control<WinButton>(this,title);
	_windows[wid] = b;
	_winPanel->add(b);
	_winPanel->layout();
	_winPanel->repaint();
}

void DesktopWin::onWindowActive(gwinid_t wid) {
	if(_active) {
		_active->getTheme().unsetColor(Theme::BTN_BACKGROUND);
		_active->repaint();
	}
	auto it = _windows.find(wid);
	if(it != _windows.end()) {
		const Theme *def = Application::getInstance()->getDefaultTheme();
		_active = it->second.get();
		_active->getTheme().setColor(Theme::BTN_BACKGROUND,def->getColor(Theme::WIN_TITLE_ACT_BG));
		_active->repaint();
	}
	else
		_active = nullptr;
}

void DesktopWin::onWindowDestroyed(gwinid_t wid) {
	auto it = _windows.find(wid);
	if(it != _windows.end()) {
		shared_ptr<WinButton> b = (*it).second;
		_winPanel->remove(b);
		if(_active == b.get())
			_active = nullptr;
		_windows.erase(wid);
		_winPanel->layout();
		_winPanel->repaint();
	}
}
