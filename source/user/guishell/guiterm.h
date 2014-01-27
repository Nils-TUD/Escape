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

#include <esc/common.h>
#include <vterm/vtctrl.h>

#include "shellcontrol.h"

class GUITerm {
private:
	/**
	 * The min-size of the buffer before we pass it to the shell-control
	 */
	static const size_t UPDATE_BUF_SIZE	= 256;
	/**
	 * The total size of the buffer
	 */
	static const size_t READ_BUF_SIZE	= 512;
	/**
	 * The size of the buffer for the read/write data
	 */
	static const size_t CLIENT_BUF_SIZE	= 4096;

public:
	GUITerm(int sid,std::shared_ptr<ShellControl> sh,uint cols,uint rows);
	virtual ~GUITerm();

	void run();
	void stop() {
		_run = false;
	}

private:
	// no cloning
	GUITerm(const GUITerm& gt);
	GUITerm& operator=(const GUITerm& gt);

	void read(int fd,sMsg *msg);
	void write(int fd,sMsg *msg);
	void prepareMode();
	static void setCursor(sVTerm *vt);

private:
	int _sid;
	volatile bool _run;
	sVTerm *_vt;
	sScreenMode _mode;
	std::shared_ptr<ShellControl> _sh;
	char *_rbuffer;
	char *_buffer;
	size_t _rbufPos;
};
