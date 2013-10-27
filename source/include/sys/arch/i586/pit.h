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
#include <sys/arch/i586/ports.h>

class PIT {
	PIT() = delete;

	static const uint PORT_DATA				= 0x40;
	static const uint PORT_CTRL 			= 0x43;

	/* read/write mode */
	enum {
		CTRL_RWLO		= 0x10,	/* low byte only */
		CTRL_RWHI		= 0x20,	/* high byte only */
		CTRL_RWLOHI		= 0x30,	/* low byte first, then high byte */
	};

	/* timer mode */
	enum Mode {
		CTRL_MODE1 		= 0x02,	/* programmable one shot */
		CTRL_MODE2 		= 0x04,	/* rate generator */
		CTRL_MODE3 		= 0x06,	/* square wave generator */
		CTRL_MODE4 		= 0x08,	/* software triggered strobe */
		CTRL_MODE5 		= 0x0A,	/* hardware triggered strobe */
	};

	/* count mode */
	enum {
		CTRL_CNTBIN16 	= 0x00, /* binary 16 bit */
		CTRL_CNTBCD 	= 0x01,	/* BCD */
	};

public:
	static const uint BASE_FREQUENCY		= 1193182;

	/* counter to select */
	enum Channel {
		CHAN0			= 0x00,
		CHAN1			= 0x40,
		CHAN2			= 0x80,
	};

	static void enableOneShot(Channel chan,ushort divisor) {
		enable(chan,CTRL_MODE1,divisor);
	}
	static void enablePeriodic(Channel chan,ushort divisor) {
		enable(chan,CTRL_MODE2,divisor);
	}

	static uint32_t getCounter(Channel chan) {
		Ports::out<uint8_t>(PORT_CTRL,chan);
		uint32_t left = Ports::in<uint8_t>(port(chan));
		left |= Ports::in<uint8_t>(port(chan)) << 8;
		return left;
	}

private:
	static void enable(Channel chan,Mode mode,ushort divisor) {
		Ports::out<uint8_t>(PORT_CTRL,chan | CTRL_RWLOHI | mode | CTRL_CNTBIN16);
		Ports::out<uint8_t>(port(chan),divisor & 0xFF);
		Ports::out<uint8_t>(port(chan),divisor >> 8);
	}
	static int port(Channel chan) {
		return PORT_DATA + (chan >> 6);
	}
};
