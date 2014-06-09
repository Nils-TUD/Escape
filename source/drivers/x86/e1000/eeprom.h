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

#include <esc/common.h>

#include "e1000dev.h"

class EEPROM {
	EEPROM() = delete;

	static const size_t WORD_LEN_LOG2	= 1;
	static const uint MAX_WAIT_MS		= 100;

public:
	static int init(E1000 *e1000);
	static int read(E1000 *e1000,uintptr_t address,uint8_t *data,size_t len);

private:
	static int readWord(E1000 *e1000,uintptr_t address,uint8_t *data);

	static int shift;
	static uint32_t doneBit;
};
