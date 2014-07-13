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
#include <esc/ipc/vtermdevice.h>

#include "shellcontrol.h"

class GUIVTermDevice : public ipc::VTermDevice {
	/**
	 * The total size of the buffer
	 */
	static const size_t READ_BUF_SIZE	= 512;

public:
	explicit GUIVTermDevice(const char *path,mode_t mode,
		std::shared_ptr<ShellControl> sh,uint cols,uint rows);
	virtual ~GUIVTermDevice();

	void loop();

private:
	virtual void setVideoMode(int) {
		VTHROW("Not supported");
	}
	virtual void update() {
		/* we do the update after every request anyway */
	}

	void write(ipc::IPCStream &is);
	void getMode(ipc::IPCStream &is);
	void getModes(ipc::IPCStream &is);

	void prepareMode();
	static void setCursor(sVTerm *vt);

	sVTerm _vt;
	ipc::Screen::Mode _mode;
	std::shared_ptr<ShellControl> _sh;
	char *_rbuffer;
	size_t _rbufPos;
};
