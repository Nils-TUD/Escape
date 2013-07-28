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

#pragma once

#include <sys/common.h>

class Serial {
	Serial() = delete;

	enum {
		DLR_LO	= 0,
		DLR_HI	= 1,
		IER		= 1,	/* interrupt enable register */
		FCR		= 2,	/* FIFO control register */
		LCR		= 3,	/* line control register */
		MCR		= 4,	/* modem control register */
	};

public:
	enum {
		COM1	= 0,
		COM2	= 1,
		COM3	= 2,
		COM4	= 3
	};

	/**
	 * Inits the serial ports
	 */
	static void init();

	/**
	 * Writes the given byte to the given serial port
	 *
	 * @param port the port
	 * @param byte the byte
	 */
	static void out(uint16_t port,uint8_t byte);

private:
	static void debugc(char c) asm("debugc");
	static int isTransmitEmpty(uint16_t port);
	static void initPort(uint16_t port);

	static const uint16_t ports[];
	static klock_t lock;
};
