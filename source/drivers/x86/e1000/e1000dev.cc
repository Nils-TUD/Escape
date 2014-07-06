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
#include <esc/arch.h>
#include <esc/conf.h>
#include <esc/thread.h>
#include <esc/irq.h>
#include <stdlib.h>

#include "e1000dev.h"
#include "eeprom.h"

E1000::E1000(ipc::PCI &pci,const ipc::PCI::Device &nic)
		: NICDriver(), _irq(nic.irq), _irqsem(), _curRxBuf(), _curTxBuf(), _bufs(),
		  _bufsPhys(), _mmio(), _handler() {
	if(_irqsem < 0)
		error("Unable to create irq-semaphore");

	// map MMIO region
	for(size_t i = 0; i < ARRAY_SIZE(ipc::PCI::Device::bars); ++i) {
		if(nic.bars[i].addr && nic.bars[i].type == ipc::PCI::Bar::BAR_MEM) {
			uintptr_t phys = nic.bars[i].addr;
			_mmio = reinterpret_cast<volatile uint32_t*>(mmapphys(&phys,nic.bars[i].size,0,MAP_PHYS_MAP));
			if(_mmio == NULL)
				error("Unable to map MMIO region %p..%p",phys,phys + nic.bars[i].size - 1);
			print("Mapped MMIO region %p..%p @ %p",phys,phys + nic.bars[i].size - 1,_mmio);
			break;
		}
	}

	// create buffers in contiguous physical memory
	uintptr_t phys = 0;
	_bufs = reinterpret_cast<Buffers*>(mmapphys(&phys,sizeof(Buffers),PAGESIZE,MAP_PHYS_ALLOC));
	if(_bufs == NULL)
		error("Unable to map buffer space of %zu bytes",sizeof(Buffers));
	print("Mapped buffer space @ virt=%p phys=%p",_bufs,phys);
	_bufsPhys = reinterpret_cast<Buffers*>(phys);

	// clear descriptors
	memset(_bufs->rxDescs,0,sizeof(_bufs->rxDescs));
	memset(_bufs->txDescs,0,sizeof(_bufs->txDescs));

	// reset card
	reset();

	// create the IRQ sem here to ensure that we've registered it if the first interrupt arrives
	if(pci.hasCap(nic.bus,nic.dev,nic.func,ipc::PCI::CAP_MSI)) {
		DBG1("Using MSIs (%u)",nic.irq);
		uint64_t msiaddr;
		uint32_t msival;
		_irqsem = semcrtirq(nic.irq,"E1000",&msiaddr,&msival);
		if(_irqsem < 0)
			error("Unable to create irq-semaphore");

		pci.enableMSIs(nic.bus,nic.dev,nic.func,msiaddr,msival);
	}
	else {
		DBG1("Using legacy IRQs (%u)",nic.irq);
		_irqsem = semcrtirq(nic.irq,"E1000",NULL,NULL);
		if(_irqsem < 0)
			error("Unable to create irq-semaphore");
	}

	// ensure that interrupts are enabled for the PCI device
	uint32_t statusCmd = pci.read(nic.bus,nic.dev,nic.func,0x04);
	pci.write(nic.bus,nic.dev,nic.func,0x04,statusCmd & ~0x400);

	// enable interrupts
	writeReg(REG_IMC,ICR_LSC | ICR_RXO | ICR_RXT0);
	writeReg(REG_IMS,ICR_LSC | ICR_RXO | ICR_RXT0);

	if(startthread(irqThread,this) < 0)
		error("Unable to start receive-thread");
}

void E1000::readEEPROM(uint8_t *dest,size_t len) {
	int err;
	if((err = EEPROM::init(this)) != 0) {
		print("Unable to init EEPROM: %s",strerror(err));
		return;
	}

	if((err = EEPROM::read(this,0x0,dest,len)) != 0) {
		print("Unable to read from EEPROM: %s",strerror(err));
		return;
	}
}

ipc::NIC::MAC E1000::readMAC() {
	uint8_t bytes[ipc::NIC::MAC::LEN];

	// read current address from RAL/RAH
	uint32_t macl,mach;
	macl = readReg(REG_RAL);
	mach = readReg(REG_RAH);
	ipc::NIC::MAC macaddr(
		(macl >>  0) & 0xFF,
		(macl >>  8) & 0xFF,
		(macl >> 16) & 0xFF,
		(macl >> 24) & 0xFF,
		(mach >>  0) & 0xFF,
		(mach >>  8) & 0xFF
	);
	DBG1("Got MAC %02x:%02x:%02x:%02x:%02x:%02x from RAL/RAH",
		macaddr.bytes()[0],macaddr.bytes()[1],macaddr.bytes()[2],
		macaddr.bytes()[3],macaddr.bytes()[4],macaddr.bytes()[5]);

	// if thats valid, take it
	if(macaddr != ipc::NIC::MAC::broadcast() && macaddr.value() != 0)
		return macaddr;

	DBG1("Reading MAC from EEPROM");
	readEEPROM(bytes,sizeof(bytes));
	return ipc::NIC::MAC(bytes);
}

void E1000::reset() {
	// force RX and TX packet buffer allocation, to work around an errata in ICH devices.
	uint32_t pbs = readReg(REG_PBS);
	if(pbs == 0x14 || pbs == 0x18) {
		DBG1("WARNING: applying ICH PBS/PBA errata");
		writeReg(REG_PBA,0x08);
		writeReg(REG_PBS,0x10);
	}

	// always reset MAC.  Required to reset the TX and RX rings.
	uint32_t ctrl = readReg(REG_CTRL);
	writeReg(REG_CTRL,ctrl | CTL_RESET);
	sleep(20);

	// set a sensible default configuration
	ctrl |= CTL_SLU | CTL_ASDE;
	ctrl &= ~(CTL_LRST | CTL_FRCSPD | CTL_FRCDPLX);
	writeReg(REG_CTRL,ctrl);
	sleep(20);

	// if link is already up, do not attempt to reset the PHY.  On
	// some models (notably ICH), performing a PHY reset seems to
	// drop the link speed to 10Mbps.
	uint32_t status = readReg(REG_STATUS);
	if(~status & STATUS_LU) {
		// Reset PHY and MAC simultaneously
		writeReg(REG_CTRL,ctrl | CTL_RESET | CTL_PHY_RESET);
		sleep(20);

		// PHY reset is not self-clearing on all models
		writeReg(REG_CTRL,ctrl);
		sleep(20);
	}

	// init receive ring
	writeReg(REG_RDBAH,0);
	writeReg(REG_RDBAL,reinterpret_cast<uintptr_t>(_bufsPhys->rxDescs + 0));
	writeReg(REG_RDLEN,RX_BUF_COUNT * sizeof(RxDesc));
	writeReg(REG_RDH,0);
	writeReg(REG_RDT,RX_BUF_COUNT - 1);
	writeReg(REG_RDTR,0);
	writeReg(REG_RADV,0);

	// init transmit ring
	writeReg(REG_TDBAH,0);
	writeReg(REG_TDBAL,reinterpret_cast<uintptr_t>(_bufsPhys->txDescs + 0));
	writeReg(REG_TDLEN,TX_BUF_COUNT * sizeof(TxDesc));
	writeReg(REG_TDH,0);
	writeReg(REG_TDT,0);
	writeReg(REG_TIDV,0);
	writeReg(REG_TADV,0);

	// setup rx descriptors
	for(size_t i = 0; i < RX_BUF_COUNT; i++) {
		_bufs->rxDescs[i].length = RX_BUF_SIZE;
		_bufs->rxDescs[i].buffer = reinterpret_cast<uint64_t>(_bufsPhys->rxBuf + i * RX_BUF_SIZE);
	}

	// enable rings
	writeReg(REG_RDCTL,readReg(REG_RDCTL) | XDCTL_ENABLE);
	writeReg(REG_TDCTL,readReg(REG_TDCTL) | XDCTL_ENABLE);

	// get MAC and setup MAC filter
	_mac = readMAC();
	uint64_t macval = _mac.value();
	writeReg(REG_RAL,macval & 0xFFFFFFFF);
	writeReg(REG_RAH,((macval >> 32) & 0xFFFF) | RAH_VALID);

	// enable transmitter
	uint32_t tctl = readReg(REG_TCTL);
	tctl &= ~(TCTL_COLT_MASK | TCTL_COLD_MASK);
	tctl |= TCTL_ENABLE | TCTL_PSP | TCTL_COLL_DIST | TCTL_COLL_TSH;
	writeReg(REG_TCTL,tctl);

	// enable receiver
	uint32_t rctl = readReg(REG_RCTL);
	rctl &= ~(RCTL_BSIZE_MASK | RCTL_BSEX_MASK);
	rctl |= RCTL_ENABLE | RCTL_UPE | RCTL_MPE | RCTL_BAM | RCTL_BSIZE_2K | RCTL_SECRC;
	writeReg(REG_RCTL,rctl);
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
		DBG1("No free buffers");
		return -EBUSY;
	}

	// copy to buffer
	memcpy(_bufs->txBuf + cur * TX_BUF_SIZE,packet,size);

	uint8_t *phys = _bufsPhys->txBuf + cur * TX_BUF_SIZE;
	DBG2("TX %u: %p..%p",cur,phys,phys + size);

	// setup descriptor
	_bufs->txDescs[cur].cmd = TX_CMD_EOP | TX_CMD_IFCS;
	_bufs->txDescs[cur].length = size;
	_bufs->txDescs[cur].buffer = reinterpret_cast<uint64_t>(phys);
	_bufs->txDescs[cur].status = 0;
	asm volatile ("" : : : "memory");

	writeReg(REG_TDT,_curTxBuf);
	return size;
}

void E1000::receive() {
	uint32_t head = readReg(REG_RDH);
	while(_curRxBuf != head) {
		RxDesc *desc = _bufs->rxDescs + _curRxBuf;

		if(~desc->status & RDS_DONE)
			break;

		DBG2("RX %u: %#08Lx..%#08Lx st=%#02x err=%#02x",
			_curRxBuf,desc->buffer,desc->buffer + desc->length,desc->status,desc->error);

		// read data into packet
		size_t size = desc->length;
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
		if(icr & (ICR_RXT0 | ICR_RXO))
			e1000->receive();
		else
			printe("Unexpected interrupt: %#08x",icr);
	}
	return 0;
}
