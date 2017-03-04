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

#include <esc/proto/ui.h>

namespace esc {

class RichUI : public UI {
public:
	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @throws if the open failed
	 */
	explicit RichUI(const char *path) : UI(path) {
	}
	/**
	 * Attaches this object to the given file-descriptor
	 *
	 * @param f the file-descriptor
	 */
	explicit RichUI(int f) : UI(f) {
	}

	/**
	 * Requests the given video-mode.
	 *
	 * @param mode the video-mode
	 * @throws if the operation failed
	 */
	void requestMode(int mode) {
		errcode_t res;
		_is << mode << SendReceive(MSG_UIM_SETMODE) >> res;
		if(res < 0)
			VTHROWE("setMode(" << mode << ")",res);
	}
};

}
