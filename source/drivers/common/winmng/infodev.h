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

#include <esc/ipc/filedev.h>
#include <sys/common.h>
#include <sys/thread.h>
#include <stdlib.h>

class InfoDev : public esc::FileDevice {
public:
	static void create(const char *ui) {
		char tmp[MAX_PATH_LEN];
		snprintf(tmp,sizeof(tmp),"/sys/%s-windows",ui);
		new InfoDev(tmp,0444);
	}

	explicit InfoDev(const char *path,mode_t mode)
		: esc::FileDevice(path,mode) {
		if(startthread(thread,this) < 0)
			error("Unable to create info thread");
	}

	virtual std::string handleRead();

private:
	static int thread(void *arg);
};
