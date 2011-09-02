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
#include <esc/debug.h>

#define IOPORT_KB_DATA				0x60
#define IOPORT_KB_CTRL				0x64
#define KBC_CMD_RESET				0xFE

void reboot_arch(void) {
	if(requestIOPort(IOPORT_KB_DATA) < 0)
		error("Unable to request io-port %d",IOPORT_KB_DATA);
	if(requestIOPort(IOPORT_KB_CTRL) < 0)
		error("Unable to request io-port %d",IOPORT_KB_CTRL);

	debugf("Using pulse-reset line of 8042 controller to reset...\n");
	/* wait until in-buffer empty */
	while((inByte(IOPORT_KB_CTRL) & 0x2) != 0)
		;
	/* command 0xD1 to write the outputport */
	outByte(IOPORT_KB_CTRL,0xD1);
	/* wait again until in-buffer empty */
	while((inByte(IOPORT_KB_CTRL) & 0x2) != 0)
		;
	/* now set the new output-port for reset */
	outByte(IOPORT_KB_DATA,0xFE);
}

void shutdown_arch(void) {
	debugf("System is stopped\n");
	debugf("You can turn off now\n");
	/* TODO we should use ACPI later here */
}
