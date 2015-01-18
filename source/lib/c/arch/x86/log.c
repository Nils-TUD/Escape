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

#include <sys/arch/x86/ports.h>
#include <sys/common.h>
#include <sys/log.h>
#include <assert.h>

static bool reqPorts = false;

void logc(char c) {
	if(!reqPorts) {
		/* request io-ports for qemu and bochs */
		sassert(reqport(0xe9) >= 0);
		sassert(reqport(0x3f8) >= 0);
		sassert(reqport(0x3fd) >= 0);
		reqPorts = true;
	}
	while((inbyte(0x3f8 + 5) & 0x20) == 0)
		;
	outbyte(0x3f8,c);
}
