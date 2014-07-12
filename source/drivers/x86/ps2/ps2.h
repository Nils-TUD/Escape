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

#include <sys/common.h>

class PS2 {
	PS2() = delete;

public:
	enum {
		PORT_DATA				= 0x60,
		PORT_CTRL				= 0x64,
	};

	enum {
		ST_OUTBUF_FULL			= 1 << 0,	// must be set before attempting to read data
		ST_INBUF_FULL			= 1 << 1,	// must be clear before attempting to write data
		ST_SYSTEM				= 1 << 2,	// meant to be cleared on reset and set by firmware
		ST_COMMAND				= 1 << 3,	// 0 = data for PS/2 device, 1 = data for PS/2 controller
		ST_OUTBUF_FULL_PORT2	= 1 << 5,	// outbuf full for PS/2 port 2 (actually, it's chipset specific)
		ST_TIMEOUT				= 1 << 6,	// 1 = timeout error
		ST_PARITY				= 1 << 7,	// 1 = parity error
	};

	enum {
		CFG_IRQ_PORT1			= 1 << 0,
		CFG_IRQ_PORT2			= 1 << 1,
		CFG_SYSTEM				= 1 << 2,
		CFG_CLOCK_PORT1			= 1 << 4,
		CFG_CLOCK_PORT2			= 1 << 5,
		CFG_TRANSLATION			= 1 << 6,
	};

	enum {
		CTRL_CMD_GETCFG			= 0x20,
		CTRL_CMD_SETCFG			= 0x60,
		CTRL_CMD_DIS_PORT2		= 0xA7,
		CTRL_CMD_EN_PORT2		= 0xA8,
		CTRL_CMD_TEST_PORT1		= 0xA9,
		CTRL_CMD_TEST			= 0xAA,
		CTRL_CMD_TEST_PORT2		= 0xAB,
		CTRL_CMD_DIS_PORT1		= 0xAD,
		CTRL_CMD_EN_PORT1		= 0xAE,
		CTRL_CMD_NEXT_PORT2_OUT	= 0xD3,
		CTRL_CMD_NEXT_PORT2_IN	= 0xD4,
	};

	enum {
		PORT1					= 1 << 0,
		PORT2					= 1 << 1,
	};

	enum {
		IRQ_KB					= 1,
		IRQ_MOUSE				= 12,
	};

	enum {
		TIMEOUT					= 10,	// number of tries to read the reply
	};

	static void init();

	static void ctrlCmd(uint8_t cmd);
	static void ctrlCmd(uint8_t cmd,uint8_t data);
	static uint8_t ctrlCmdResp(uint8_t cmd);
	static uint8_t ctrlRead();

	static uint8_t devCmd(uint8_t cmd,uint8_t port);

private:
	static bool waitInbuf();
	static bool waitOutbuf();

	static uint _ports;
};
