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

#include <esc/common.h>
#include <esc/mem.h>
#include <esc/conf.h>
#include <esc/thread.h>
#include <esc/irq.h>
#include <stdlib.h>

#include "e1000dev.h"

E1000::E1000(ipc::PCI &pci,const ipc::PCI::Device &nic)
		: _irq(nic.irq), _irqsem(semcrtirq(_irq,"E1000")), _curRxBuf(), _curTxBuf(), _bufs(),
		  _bufsPhys(), _mmio(), _handler() {
	// create the IRQ sem here to ensure that we've registered it if the first interrupt arrives
	if(_irqsem < 0)
		error("Unable to create irq-semaphore");

	if(startthread(irqThread,this) < 0)
		error("Unable to start receive-thread");

	// map MMIO region
	for(size_t i = 0; i < ARRAY_SIZE(ipc::PCI::Device::bars); ++i) {
		if(nic.bars[i].addr && nic.bars[i].type == ipc::PCI::Bar::BAR_MEM) {
			uintptr_t phys = nic.bars[i].addr;
			_mmio = reinterpret_cast<volatile uint32_t*>(mmapphys(&phys,nic.bars[i].size,0));
			if(_mmio == NULL)
				error("Unable to map MMIO region %p..%p",phys,phys + nic.bars[i].size - 1);
			print("Mapped MMIO region %p..%p @ %p",phys,phys + nic.bars[i].size - 1,_mmio);
		}
	}

	// create buffers in contiguous physical memory
	uintptr_t phys = 0;
	size_t pageSize = sysconf(CONF_PAGE_SIZE);
	_bufs = reinterpret_cast<Buffers*>(mmapphys(&phys,sizeof(Buffers),pageSize));
	if(_bufs == NULL)
		error("Unable to map buffer space of %zu bytes",sizeof(Buffers));
	print("Mapped buffer space @ virt=%p phys=%p",_bufs,phys);
	_bufsPhys = reinterpret_cast<Buffers*>(phys);

	// clear descriptors
	memset(_bufs->rxDescs,0,sizeof(_bufs->rxDescs));
	memset(_bufs->txDescs,0,sizeof(_bufs->txDescs));

	// reset card
	reset();

	// ensure that interrupts are enabled for the PCI device
	uint32_t statusCmd = pci.read(nic.bus,nic.dev,nic.func,0x04);
	pci.write(nic.bus,nic.dev,nic.func,0x04,statusCmd & ~0x400);

	// enable interrupts
	writeReg(REG_IMC,0xFFFF);
	writeReg(REG_IMS,0xFFFF);
}

uint16_t E1000::readEEPROM(size_t offset) {
	uint32_t eerd;
	writeReg(REG_EERD,(offset << 8) | EERD_START);
	for(int i = 0; i < 100; i++) {
		eerd = readReg(REG_EERD);
		if(eerd & EERD_DONE)
			break;
		sleep(1);
	}

	if(eerd & EERD_DONE)
		return (eerd >> 16) & 0xFFFF;
	return -1;
}

ipc::NIC::MAC E1000::readMAC() {
	uint8_t bytes[ipc::NIC::MAC::LEN];
	for(size_t i = 0; i < 3; i++) {
		uint16_t word = readEEPROM(EEPROM_OFS_MAC + i);
		bytes[i * 2] = word & 0xFF;
		bytes[i * 2 + 1] = word >> 8;
	}
	return ipc::NIC::MAC(bytes);
}

void E1000::reset() {
	// disable sending/receiving
	writeReg(REG_RCTL,0);
	writeReg(REG_TCTL,0);

	// reset
	writeReg(REG_CTRL,CTL_PHY_RESET);
	sleep(10);
	writeReg(REG_CTRL,CTL_RESET);
	sleep(10);
	while(readReg(REG_CTRL) & CTL_RESET)
		;

	// init control registers
	writeReg(REG_CTRL,CTL_AUTO_SPEED | CTL_LINK_UP);

	// init receive ring
	writeReg(REG_RDBAH,0);
	writeReg(REG_RDBAL,reinterpret_cast<uint32_t>(_bufsPhys->rxDescs + 0));
	writeReg(REG_RDLEN,RX_BUF_COUNT * sizeof(RxDesc));
	writeReg(REG_RDH,0);
	writeReg(REG_RDT,RX_BUF_COUNT - 1);
	writeReg(REG_RDTR,0);
	writeReg(REG_RADV,0);

	// init transmit ring
	writeReg(REG_TDBAH,0);
	writeReg(REG_TDBAL,reinterpret_cast<uint32_t>(_bufsPhys->txDescs + 0));
	writeReg(REG_TDLEN,TX_BUF_COUNT * sizeof(TxDesc));
	writeReg(REG_TDH,0);
	writeReg(REG_TDT,0);
	writeReg(REG_TIDV,0);
	writeReg(REG_TADV,0);

	// disable VLANs
	writeReg(REG_VET,0);

	// get MAC and setup MAC filter
	_mac = readMAC();
	uint64_t macval = _mac.value();
	writeReg(REG_RAL,macval & 0xFFFFFFFF);
	writeReg(REG_RAH,((macval >> 32) & 0xFFFF) | RAH_VALID);

	// setup rx descriptors
	for(size_t i = 0; i < RX_BUF_COUNT; i++) {
		_bufs->rxDescs[i].length = RX_BUF_SIZE;
		_bufs->rxDescs[i].buffer = reinterpret_cast<uint64_t>(_bufsPhys->rxBuf + i * RX_BUF_SIZE);
	}

	// enable sending/receiving
	writeReg(REG_RCTL,RCTL_ENABLE | RCTL_BROADCAST | RCTL_BUFSIZE_2K);
	writeReg(REG_TCTL,TCTL_ENABLE | TCTL_COLL_TSH | TCTL_COLL_DIST);
}

ssize_t E1000::send(const void *packet,size_t size) {
	assert(size <= mtu());
	// to next tx descriptor
	uint32_t cur = _curTxBuf;
	_curTxBuf = (_curTxBuf + 1) % TX_BUF_COUNT;

	// is there enough space?
	uint32_t head = readReg(REG_TDH);
	if(_curTxBuf == head) {
		// TODO handle that case
		return -EBUSY;
	}

	// copy to buffer
	memcpy(_bufs->txBuf + cur * TX_BUF_SIZE,packet,size);

	// setup descriptor
	_bufs->txDescs[cur].cmd = TX_CMD_EOP | TX_CMD_IFCS;
	_bufs->txDescs[cur].length = size;
	_bufs->txDescs[cur].buffer = reinterpret_cast<uint64_t>(_bufsPhys->txBuf + cur * TX_BUF_SIZE);

	writeReg(REG_TDT,_curTxBuf);
	return size;
}

void E1000::receive() {
	uint32_t head = readReg(REG_RDH);
	while(_curRxBuf != head) {
		size_t size = _bufs->rxDescs[_curRxBuf].length;
		uint8_t status = _bufs->rxDescs[_curRxBuf].status;

		if(~status & RDS_DONE)
			break;

		// 4 bytes CRC
		size -= 4;

		// read data into packet
		Packet *pkt = (Packet*)malloc(sizeof(Packet) + size);
		if(!pkt) {
			printe("Not enough memory to read packet");
			break;
		}
		pkt->length = size;
		memcpy(pkt->data,_bufs->rxBuf + _curRxBuf * RX_BUF_SIZE,size);

		// insert into list
		insert(pkt);
		(*_handler)();

		// to next packet
		_curRxBuf = (_curRxBuf + 1) % RX_BUF_COUNT;
	}

	// set new tail
	if(_curRxBuf == head)
		writeReg(REG_RDT,(head + RX_BUF_COUNT - 1) % RX_BUF_COUNT);
	else
		writeReg(REG_RDT,_curRxBuf);
}

int E1000::irqThread(void *ptr) {
	E1000 *e1000 = reinterpret_cast<E1000*>(ptr);
	while(1) {
		semdown(e1000->_irqsem);

		// bits 31:17 are reserved
		uint32_t icr = e1000->readReg(REG_ICR) & 0x1FFFF;

		// packet received
		if(icr & ICR_RECEIVE)
			e1000->receive();
		else if(icr & ICR_TXQE)
			;
		else
			printe("Unexpected interrupt: %#08x",icr);
	}
	return 0;
}
