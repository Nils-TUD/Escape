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
#include <sys/debug.h>

#include "serial.h"

enum {
	DLR_LO	= 0,
	DLR_HI	= 1,
	IER		= 1,	/* interrupt enable register */
	FCR		= 2,	/* FIFO control register */
	LCR		= 3,	/* line control register */
	MCR		= 4,	/* modem control register */
};

static const int port = 0x3F8;

static void outbyte(uint16_t port,uint8_t val) {
	__asm__ volatile (
		"out	%%al,%%dx"
		: : "a"(val), "d"(port)
	);
}

static uint8_t inbyte(uint16_t port) {
	uint8_t res;
	__asm__ volatile (
		"in	%%dx,%%al"
		: "=a"(res) : "d"(port)
	);
	return res;
}

extern "C" void debugChar(char c) {
	while((inbyte(port + 5) & 0x20) == 0)
		;
	outbyte(port,c);
}

void initSerial(void) {
	outbyte(port + LCR,0x80);		/* Enable DLAB (set baud rate divisor) */
	outbyte(port + DLR_LO,0x01);	/* Set divisor to 1 (lo byte) 115200 baud */
	outbyte(port + DLR_HI,0x00);	/*                  (hi byte) */
	outbyte(port + LCR,0x03);	/* 8 bits, no parity, one stop bit */
	outbyte(port + IER,0);		/* disable interrupts */
	outbyte(port + FCR,7);
	outbyte(port + MCR,3);
}
