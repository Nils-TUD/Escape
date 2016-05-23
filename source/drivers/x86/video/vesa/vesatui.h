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

#include "vesascreen.h"

class VESATUI {
	static const gpos_t CURSOR_LEN			= FONT_WIDTH;

public:
	void drawChars(VESAScreen *scr,gpos_t col,gpos_t row,const uint8_t *str,size_t len);
	void setCursor(VESAScreen *scr,gpos_t col,gpos_t row);

private:
	void drawChar(VESAScreen *scr,gpos_t col,gpos_t row,uint8_t c,uint8_t color);
	void drawCursor(VESAScreen *scr,gpos_t col,gpos_t row,uint8_t color);
};
