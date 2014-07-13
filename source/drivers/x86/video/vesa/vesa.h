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

#include "vesascreen.h"
#include "../vbe.h"

class VESA {
	VESA() = delete;

	class Client : public ipc::ScreenClient {
	public:
		explicit Client(int f) : ScreenClient(f), screen() {
		}

		sVESAScreen *screen;
	};

	class ScreenDevice : public ipc::ScreenDevice<Client> {
	public:
		explicit ScreenDevice(const std::vector<ipc::Screen::Mode> &modes,const char *path,mode_t mode)
			: ipc::ScreenDevice<Client>(modes,path,mode) {
		}

		virtual void setScreenMode(Client *c,const char *shm,ipc::Screen::Mode *mode,int type,bool sw);
		virtual void setScreenCursor(Client *c,gpos_t x,gpos_t y,int cursor);
		virtual void updateScreen(Client *c,gpos_t x,gpos_t y,gsize_t width,gsize_t height);
	};

public:
	static void init();
	static int run(void *arg);

private:
	static std::vector<ipc::Screen::Mode> modes;
};
