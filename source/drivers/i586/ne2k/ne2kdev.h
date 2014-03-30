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
#include <esc/arch/i586/ports.h>
#include <esc/messages.h>
#include <esc/sync.h>
#include <esc/irq.h>

class Ne2k {
	struct Packet {
		Packet *next;
		size_t length;
		uint16_t data[];
	};

	enum Mode {
		PROM_READ,
		PROM_WRITE
	};

public:
	/* TODO wrong place */
	static const unsigned NIC_CLASS		= 0x02;
	static const unsigned NIC_SUBCLASS	= 0x00;

	static const unsigned VENDOR_ID		= 0x10ec;
	static const unsigned DEVICE_ID		= 0x8029;

	explicit Ne2k(sPCIDevice *nic,int sid);

	const uint8_t *mac() const {
		return _mac;
	}

	ssize_t send(const void *packet,size_t size);
	ssize_t fetch(void *buffer,size_t size);

private:
	static int irqThread(void*);

	void accessPROM(uint16_t offset,size_t size,void *buffer,Mode mode);
	void receive();

	void writeReg(uint16_t reg,uint8_t value) {
		outbyte(_basePort + reg,value);
	}
	void writeReg16(uint16_t reg,uint16_t value) {
		outword(_basePort + reg,value);
	}
	uint8_t readReg(uint16_t reg) {
		return inbyte(_basePort + reg);
	}
	uint16_t readReg16(uint16_t reg) {
		return inword(_basePort + reg);
	}

	int _sid;
	uint16_t _basePort;
	uint8_t _mac[6];
	uint8_t _nextPacket;
	tUserSem _listsem;
	Packet *_first;
	Packet *_last;
};
