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

#include <esc/proto/screen.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <vector>

#include "clients.h"

/**
 * Manages the screens the uimng uses
 */
class ScreenMng {
public:
	/**
	 * Inits the screens from given device-names
	 *
	 * @param cnt the number of available screens
	 * @param names the names of the devices that provide them
	 */
	static void init(int cnt,char *names[]);

	/**
	 * Finds a screen with given mode-id
	 *
	 * @param mid the mode-id
	 * @param mode will be set to the mode-information
	 * @param scr will be set to the Screen instance
	 * @return true if found
	 */
	static bool find(int mid,esc::Screen::Mode *mode,esc::Screen **scr);

	/**
	 * Adjusts the available size of the given mode, i.e. the header is removed.
	 *
	 * @param mode the mode
	 */
	static void adjustMode(esc::Screen::Mode *mode);

	/**
	 * Stores the list of available modes in <modes>.
	 *
	 * @param modes the array to copy to
	 * @param n if 0, only the number of available modes will be returned. otherwise, that many modes
	 *  are copied at max
	 * @return the number of found modes
	 */
	static ssize_t getModes(esc::Screen::Mode *modes,size_t n);

private:
	static std::vector<esc::Screen*> _screens;
};
