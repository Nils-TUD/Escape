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

#include <sys/common.h>
#include <sys/arch/i586/serial.h>
#include <sys/arch/i586/ports.h>
#include <sys/spinlock.h>
#include <assert.h>

static int ser_isTransmitEmpty(uint16_t port);
static void ser_initPort(uint16_t port);

static const uint16_t ports[] = {
	/* COM1 */	0x3F8,
	/* COM2 */	0x2E8,
	/* COM3 */	0x2F8,
	/* COM4 */	0x3E8
};
static klock_t serialLock;

void ser_init(void) {
	ser_initPort(ports[SER_COM1]);
}

void ser_out(uint16_t port,uint8_t byte) {
	uint16_t ioport;
	assert(port < ARRAY_SIZE(ports));
	ioport = ports[port];
	spinlock_aquire(&serialLock);
	while(ser_isTransmitEmpty(ioport) == 0);
	ports_outByte(ioport,byte);
	spinlock_release(&serialLock);
}

static int ser_isTransmitEmpty(uint16_t port) {
	return ports_inByte(port + 5) & 0x20;
}

static void ser_initPort(uint16_t port) {
	ports_outByte(port + 1, 0x00);	/* Disable all interrupts */
	ports_outByte(port + 3, 0x80);	/* Enable DLAB (set baud rate divisor) */
	ports_outByte(port + 0, 0x03);	/* Set divisor to 3 (lo byte) 38400 baud */
	ports_outByte(port + 1, 0x00);	/*                  (hi byte) */
	ports_outByte(port + 3, 0x03);	/* 8 bits, no parity, one stop bit */
	ports_outByte(port + 2, 0xC7);	/* Enable FIFO, clear them, with 14-byte threshold */
	ports_outByte(port + 4, 0x0B);	/* IRQs enabled, RTS/DSR set */
}
