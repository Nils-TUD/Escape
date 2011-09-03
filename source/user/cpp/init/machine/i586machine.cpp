/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <esc/common.h>
#include <esc/arch/i586/ports.h>
#include "../initerror.h"
#include "i586machine.h"

void i586Machine::reboot(Progress &pg) {
	pg.itemStarting("Resetting using pulse-reset line of 8042 controller...");

	if(requestIOPort(PORT_KB_DATA) < 0)
		throw init_error("Unable to request keyboard data-port");
	if(requestIOPort(PORT_KB_CTRL) < 0)
		throw init_error("Unable to request keyboard-control-port");

	// wait until in-buffer empty
	while((inByte(PORT_KB_CTRL) & 0x2) != 0)
		;
	// command 0xD1 to write the outputport
	outByte(PORT_KB_CTRL,0xD1);
	// wait again until in-buffer empty
	while((inByte(PORT_KB_CTRL) & 0x2) != 0)
		;
	// now set the new output-port for reset
	outByte(PORT_KB_DATA,0xFE);
}

void i586Machine::shutdown(Progress &pg) {
	// TODO we should use ACPI later here
	pg.itemStarting("You can turn off now.");
}
