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

#pragma once

#include <esc/proto/input.h>
#include <sys/common.h>

class Keyboard {
	Keyboard() = delete;

	enum {
		CMD_LEDS		= 0xED,
		CMD_TYPEMATIC	= 0xF3,
	};

	union Typematic {
		uint8_t repeatRate : 5,	/* 00000b = 30 Hz, ..., 11111b = 2 Hz */
				delay : 2,		/* 00b = 250 ms, 01b = 500 ms, 10b = 750 ms, 11b = 1000 ms */
				zero : 1;		/* must be zero */
		uint8_t value;
	};

public:
	enum {
		LED_SCROLL_LOCK	= 1 << 0,
		LED_NUM_LOCK	= 1 << 1,
		LED_CAPS_LOCK	= 1 << 2,
	};

	static void init();
	static int run(void*);

private:
	static void updateLEDs(const esc::Keyb::Event &ev);
	static int irqThread(void*);

	static uint8_t _leds;
};
