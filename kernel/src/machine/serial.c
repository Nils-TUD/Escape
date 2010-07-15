/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <sys/machine/serial.h>
#include <sys/util.h>
#include <assert.h>

static int ser_isTransmitEmpty(u16 port);
static void ser_initPort(u16 port);

static u16 ports[] = {
	/* COM1 */	0x3F8,
	/* COM2 */	0x2E8,
	/* COM3 */	0x2F8,
	/* COM4 */	0x3E8
};

void ser_init(void) {
	ser_initPort(ports[SER_COM1]);
}

void ser_out(u16 port,u8 byte) {
	u16 ioport;
	assert(port < ARRAY_SIZE(ports));
	ioport = ports[port];
	while(ser_isTransmitEmpty(ioport) == 0);
	util_outByte(ioport,byte);
}

static int ser_isTransmitEmpty(u16 port) {
	return util_inByte(port + 5) & 0x20;
}

static void ser_initPort(u16 port) {
	util_outByte(port + 1, 0x00);	/* Disable all interrupts */
	util_outByte(port + 3, 0x80);	/* Enable DLAB (set baud rate divisor) */
	util_outByte(port + 0, 0x03);	/* Set divisor to 3 (lo byte) 38400 baud */
	util_outByte(port + 1, 0x00);	/*                  (hi byte) */
	util_outByte(port + 3, 0x03);	/* 8 bits, no parity, one stop bit */
	util_outByte(port + 2, 0xC7);	/* Enable FIFO, clear them, with 14-byte threshold */
	util_outByte(port + 4, 0x0B);	/* IRQs enabled, RTS/DSR set */
}
