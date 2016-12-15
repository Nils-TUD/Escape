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

class VESAScreen {
	explicit VESAScreen(esc::Screen::Mode *minfo);
    ~VESAScreen();

public:
	static VESAScreen *request(esc::Screen::Mode *minfo);

	void reset(int type);
	void release();

private:
	void initWhOnBl();

public:
	uint refs;
	uint8_t *frmbuf;
	uint8_t *whOnBlCache;
	esc::Screen::Mode *mode;
	gpos_t cols;
	gpos_t rows;
	gpos_t lastCol;
	gpos_t lastRow;
	uint8_t *content;
private:
	static std::vector<VESAScreen*> _screens;
};
