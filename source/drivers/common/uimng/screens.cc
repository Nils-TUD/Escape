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
#include <stdio.h>
#include <stdlib.h>

#include "clients.h"
#include "header.h"
#include "screens.h"

std::vector<esc::Screen*> ScreenMng::_screens;

void ScreenMng::init(int cnt,char *names[]) {
	/* open screen-devices */
	for(int i = 0; i < cnt; i++)
		_screens.push_back(new esc::Screen(names[i]));
}

bool ScreenMng::find(int mid,esc::Screen::Mode *mode,esc::Screen **scr) {
	// TODO we could cache that
	for(auto it = _screens.begin(); it != _screens.end(); ++it) {
		std::vector<esc::Screen::Mode> modes = (*it)->getModes();
		for(auto m = modes.begin(); m != modes.end(); ++m) {
			if(m->id == mid) {
				*mode = *m;
				*scr = *it;
				return true;
			}
		}
	}
	return false;
}

void ScreenMng::adjustMode(esc::Screen::Mode *mode) {
	if(mode->type & esc::Screen::MODE_TYPE_GUI) {
		mode->height -= Header::getHeight(esc::Screen::MODE_TYPE_GUI);
		mode->guiHeaderSize += Header::getHeight(esc::Screen::MODE_TYPE_GUI);
	}
	if(mode->type & esc::Screen::MODE_TYPE_TUI) {
		mode->rows -= Header::getHeight(esc::Screen::MODE_TYPE_TUI);
		mode->tuiHeaderSize += Header::getHeight(esc::Screen::MODE_TYPE_TUI);
	}
}

ssize_t ScreenMng::getModes(esc::Screen::Mode *modes,size_t n) {
	size_t count = 0;
	for(auto it = _screens.begin(); it != _screens.end(); ++it)
		count += (*it)->getModeCount();

	if(n == 0)
		return count;
	if(n != count)
		return -EINVAL;
	if(!modes)
		return -ENOMEM;

	size_t pos = 0;
	for(auto it = _screens.begin(); it != _screens.end(); ++it) {
		std::vector<esc::Screen::Mode> mlist = (*it)->getModes();
		for(auto m = mlist.begin(); m != mlist.end(); ++m) {
			adjustMode(m);
			modes[pos++] = *m;
		}
	}
	return n;
}
