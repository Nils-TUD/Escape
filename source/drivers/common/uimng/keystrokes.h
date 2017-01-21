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

#pragma once

#include <sys/common.h>
#include <sys/debug.h>
#include <sys/messages.h>
#include <mutex>

#include "clients.h"

/**
 * Handles the keystrokes the uimng provides
 */
class Keystrokes {
	Keystrokes() = delete;

	static const int VGA_MODE;

	static const char *VTERM_PROG;
	static const char *LOGIN_PROG;

	static const char *WINMNG_PROG;
	static const char *GLOGIN_PROG;

	static const char *TUI_DEF_COLS;
	static const char *TUI_DEF_ROWS;

	static const char *GUI_DEF_RES_X;
	static const char *GUI_DEF_RES_Y;

	static const char *GROUP_NAME;

public:
	/**
	 * Enters the kernel-debugger
	 */
	static void enterDebugger() {
		switchToVGA();
		debug();
		UIClient::reactivate(UIClient::getActive(),UIClient::getActive(),VGA_MODE);
	}

	/**
	 * Creates a new text-console
	 */
	static void createTextConsole() {
		createConsole(VTERM_PROG,TUI_DEF_COLS,TUI_DEF_ROWS,LOGIN_PROG,"TERM");
	}

	/**
	 * Creates a new GUI-console
	 */
	static void createGUIConsole() {
		createConsole(WINMNG_PROG,GUI_DEF_RES_X,GUI_DEF_RES_Y,GLOGIN_PROG,"WINMNG");
	}

private:
	static void createConsole(const char *mng,const char *cols,const char *rows,const char *login,
		const char *termVar);
	static void switchToVGA(void);
};
