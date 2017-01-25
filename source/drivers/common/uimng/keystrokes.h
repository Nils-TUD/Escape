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

#include <esc/proto/initui.h>
#include <sys/common.h>
#include <sys/debug.h>

#include "clients.h"

/**
 * Handles the keystrokes the uimng provides
 */
class Keystrokes {
	Keystrokes() = delete;

	static const int VGA_MODE;

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
		createConsole(esc::InitUI::TUI);
	}

	/**
	 * Creates a new GUI-console
	 */
	static void createGUIConsole() {
		createConsole(esc::InitUI::GUI);
	}

private:
	static void createConsole(esc::InitUI::Type type);
	static void switchToVGA(void);
};
