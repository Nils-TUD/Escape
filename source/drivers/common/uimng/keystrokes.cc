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

#include "keystrokes.h"
#include "screenmng.h"

const int Keystrokes::VGA_MODE			= 3;

void Keystrokes::createConsole(esc::InitUI::Type type) {
	esc::InitUI iui("/dev/initui");

	// find free UI
	int no;
	for(no = 0; no < esc::UI::MAX_UIS; ++no) {
		if(!UIClient::exists(no))
			break;
	}
	if(no == esc::UI::MAX_UIS)
		return;

	esc::OStringStream os;
	os << "ui" << no;
	iui.start(type,os.str());
}

void Keystrokes::switchToVGA() {
	esc::Screen *scr;
	esc::Screen::Mode mode;
	if(ScreenMng::find(VGA_MODE,&mode,&scr)) {
		try {
			scr->setMode(esc::Screen::MODE_TYPE_TUI,VGA_MODE,-1,true);
		}
		catch(const std::exception &e) {
			printe("Unable to switch to VGA: %s",e.what());
		}
	}
	else
		printe("Unable to find screen for mode %d",VGA_MODE);
}
