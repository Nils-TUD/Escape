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

#include <esc/ipc/filedev.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/messages.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "infodev.h"
#include "window.h"

class WinInfoDevice : public esc::FileDevice {
public:
	explicit WinInfoDevice(const char *path,mode_t mode)
		: esc::FileDevice(path,mode) {
	}

	virtual std::string handleRead() {
		std::ostringstream os;
		size_t i;
		for(i = 0; i < WINDOW_COUNT; i++) {
			Window *w = win_get(i);
			if(w) {
				os << "Window " << w->id << "\n";
				os << "\tOwner: " << w->owner << "\n";
				os << "\tPosition: " << w->x() << "," << w->y() << "," << w->z << "\n";
				os << "\tSize: " << w->width() << " x " << w->height() << "\n";
				os << "\tStyle: 0x" << std::hex << w->style << std::dec << "\n";
			}
		}
		return os.str();
	}
};

int infodev_thread(void *arg) {
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/sys/%s-windows",(const char*)arg);
	WinInfoDevice dev(path,0444);
	dev.loop();
	return 0;
}
