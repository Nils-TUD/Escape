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
#include <sys/thread.h>
#include <assert.h>

#include "eeprom.h"

int EEPROM::shift = 0;
uint32_t EEPROM::doneBit = 0;

int EEPROM::init(E1000 *e1000) {
	// determine the done bit to test when reading REG_EERD and the shift value
	e1000->writeReg(E1000::REG_EERD,E1000::EERD_START);
	for(uint i = 0; i < MAX_WAIT_MS; i++) {
		uint32_t value = e1000->readReg(E1000::REG_EERD);
		if(value & E1000::EERD_DONE_LARGE) {
			DBG1("Detected large EERD");
			doneBit = E1000::EERD_DONE_LARGE;
			shift = E1000::EERD_SHIFT_LARGE;
			return 0;
		}

		if(value & E1000::EERD_DONE_SMALL) {
			DBG1("Detected small EERD");
			doneBit = E1000::EERD_DONE_SMALL;
			shift = E1000::EERD_SHIFT_SMALL;
			return 0;
		}
		sleep(1);
	}
	return -ETIMEOUT;
}

int EEPROM::readWord(E1000 *e1000,uintptr_t address,uint8_t *data) {
	uint16_t *data_word = (uint16_t*)data;

	// set address
	e1000->writeReg(E1000::REG_EERD,E1000::EERD_START | (address << shift));

	// wait for read to complete
	for(uint i = 0; i < MAX_WAIT_MS; i++) {
		uint32_t value = e1000->readReg(E1000::REG_EERD);
		if(~value & doneBit) {
			sleep(1);
			continue;
		}

		*data_word = value >> 16;
		return 0;
	}
	return -ETIMEOUT;
}

int EEPROM::read(E1000 *e1000,uintptr_t address,uint8_t *data,size_t len) {
	assert((len & ((1 << WORD_LEN_LOG2) - 1)) == 0);
	while(len) {
		int err;
		if((err = readWord(e1000,address,data)) != 0)
			return err;

		// to next word
		data += 1 << WORD_LEN_LOG2;
		address += 1;
		len -= 1 << WORD_LEN_LOG2;
	}
	return 0;
}
