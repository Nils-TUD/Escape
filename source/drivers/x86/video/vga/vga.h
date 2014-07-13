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
#include <esc/proto/screen.h>
#include <esc/ipc/screendevice.h>
#include <vector>

#include "../vbe.h"

class VGA {
	VGA() = delete;

	class ScreenDevice : public ipc::ScreenDevice<> {
	public:
		explicit ScreenDevice(const std::vector<ipc::Screen::Mode> &modes,const char *path,mode_t mode)
			: ipc::ScreenDevice<>(modes,path,mode) {
		}

		virtual void setScreenMode(ipc::ScreenClient *c,const char *shm,ipc::Screen::Mode *mode,int type,bool sw);
		virtual void setScreenCursor(ipc::ScreenClient *c,gpos_t x,gpos_t y,int);
		virtual void updateScreen(ipc::ScreenClient *c,gpos_t x,gpos_t y,gsize_t width,gsize_t height);
	};

	enum {
		PORT_INDEX		= 0x3D4,
		PORT_DATA		= 0x3D5,
	};
	enum {
		DATA_LOCLOW		= 0x0F,
		DATA_LOCHIGH	= 0x0E,
	};

public:
	static void init();
	static int run(void *arg);

private:
	static uint8_t *screen;
	static std::vector<ipc::Screen::Mode> modes;
};
