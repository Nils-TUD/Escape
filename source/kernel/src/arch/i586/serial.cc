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
#include <sys/arch/i586/serial.h>
#include <sys/arch/i586/ports.h>
#include <sys/spinlock.h>
#include <assert.h>

const uint16_t Serial::ports[] = {
	/* COM1 */	0x3F8,
	/* COM2 */	0x2E8,
	/* COM3 */	0x2F8,
	/* COM4 */	0x3E8
};
SpinLock Serial::lock;

void Serial::init() {
	initPort(ports[COM1]);
}

void Serial::out(uint16_t port,uint8_t byte) {
	assert(port < ARRAY_SIZE(ports));
	uint16_t ioport = ports[port];
	lock.acquire();
	while(isTransmitEmpty(ioport) == 0)
		;
	Ports::out<uint8_t>(ioport,byte);
	lock.release();
}

int Serial::isTransmitEmpty(uint16_t port) {
	return Ports::in<uint8_t>(port + 5) & 0x20;
}

void Serial::initPort(uint16_t port) {
	Ports::out<uint8_t>(port + LCR,0x80);		/* Enable DLAB (set baud rate divisor) */
	Ports::out<uint8_t>(port + DLR_LO,0x01);	/* Set divisor to 1 (lo byte) 115200 baud */
	Ports::out<uint8_t>(port + DLR_HI,0x00);	/*                  (hi byte) */
	Ports::out<uint8_t>(port + LCR,0x03);	/* 8 bits, no parity, one stop bit */
	Ports::out<uint8_t>(port + IER,0);		/* disable interrupts */
	Ports::out<uint8_t>(port + FCR,7);
	Ports::out<uint8_t>(port + MCR,3);
}

void Serial::debugc(char c) {
	out(COM1,c);
}
