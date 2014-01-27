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

#include <sys/common.h>
#include <sys/arch/i586/ports.h>
#include <sys/video.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

void Video::clear() {
	memclear(screen(),VID_COLS * 2 * VID_ROWS);
	// remove cursor
	Ports::out<uint8_t>(0x3D4,14);
	Ports::out<uint8_t>(0x3D5,0x07);
	Ports::out<uint8_t>(0x3D4,15);
	Ports::out<uint8_t>(0x3D5,0xd0);
}

void Video::move() {
	/* last line? */
	if(row >= VID_ROWS) {
		/* copy all chars one line back */
		memmove(screen(),(char*)screen() + VID_COLS * 2,(VID_ROWS - 1) * VID_COLS * 2);
		memclear((char*)screen() + (VID_ROWS - 1) * VID_COLS * 2,VID_COLS * 2);
		row--;
	}
}
