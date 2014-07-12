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
#include <vbe/vbe.h>

#include "clients.h"

/**
 * Manages the header that the UI manages paints including the CPU usage and the client-list
 */
class Header {
	struct CPUUsage {
		int usage;
		char str[3];
	};

	typedef void (*putc_func)(const ipc::Screen::Mode &mode,char *header,uint *col,char c,char color);

	static const uint8_t TITLE_BAR_COLOR			= 0x1F;
	static const uint8_t CPU_USAGE_COLOR			= 0x70;
	static const uint8_t ACTIVE_CLI_COLOR			= 0x4F;
	static const uint8_t INACTIVE_CLI_COLOR			= 0x70;
	static const char *OS_TITLE;

public:
	/**
	 * Allocates the per-CPU arrays, etc.
	 */
	static void init();

	/**
	 * @return the height of the header of given type
	 */
	static size_t getHeight(int type) {
		if(type == ipc::Screen::MODE_TYPE_TUI)
			return 1;
		return FONT_HEIGHT;
	}

	/**
	 * @return the size in bytes of a x-columns wide header of given type
	 */
	static size_t getSize(const ipc::Screen::Mode &mode,int type,gpos_t x) {
		if(type == ipc::Screen::MODE_TYPE_TUI)
			return x * 2;
		/* always update the whole width because it simplifies the copy it shouldn't be much slower
		 * than doing a loop with 1 memcpy per line */
		return x * (mode.bitsPerPixel / 8) * FONT_HEIGHT;
	}

	/**
	 * Updates the header for given client
	 *
	 * @param cli the client
	 * @param width will be set to the updated width
	 * @param height will be set to the updated height
	 * @return true if an update has been performed
	 */
	static bool update(UIClient *cli,gsize_t *width,gsize_t *height);

	/**
	 * Rebuilds the header for given client, i.e. the CPU usage is re-read etc.
	 *
	 * @param cli the client
	 * @param width will be set to the updated width
	 * @param height will be set to the updated height
	 * @return true if an update has been performed
	 */
	static bool rebuild(UIClient *cli,gsize_t *width,gsize_t *height);

private:
	static void putcTUI(const ipc::Screen::Mode &,char *header,uint *col,char c,char color);
	static void putcGUI(const ipc::Screen::Mode &mode,char *header,uint *col,char c,char color);
	static void makeDirty(UIClient *cli,size_t cols,gsize_t *width,gsize_t *height);
	static void doUpdate(UIClient *cli,gsize_t width,gsize_t height);
	static void readCPUUsage();
	static void buildTitle(UIClient *cli,gsize_t *width,gsize_t *height,bool force);

	static CPUUsage *_cpuUsage;
	static CPUUsage *_lastUsage;
	static size_t _lastClientCount;
	static size_t _cpuCount;
};
