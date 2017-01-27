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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ne2kdev.h"

/*
 * The ne2k has a internal memory of 32KiB, which is split into pages of 256 bytes each.
 * It looks like this (note that this is based on the bochs model; I don't know about real HW):
 *
 *   0x8000 +---------------------+  <-- PAGE_STOP
 *          |       page 127      |
 *   0x7F00 +---------------------+
 *          |                     |
 *          |         ...         |
 *          |                     |
 *   0x5200 +---------------------+  <-- REG_CURR (write position)
 *          |       page 17       |
 *   0x5100 +---------------------+  <-- REG_BNRY (read position)
 *          |       page 16       |
 *   0x5000 +---------------------+  <-- PAGE_RX (REG_PSTART)
 *          |                     |
 *          |         ...         |
 *          |                     |
 *   0x4200 +---------------------+
 *          |       page  1       |
 *   0x4100 +---------------------+
 *          |       page  0       |
 *   0x4000 +---------------------+  <-- PAGE_TX
 *          |                     |
 *          |          ??         |
 *          |                     |
 *   0x0020 +---------------------+
 *          |     MAC address     |
 *   0x0000 +---------------------+
 *
 * So, 0x5000 .. 0x8000 is used for receiving packets, which is organized as a ringbuffer. The HW
 * uses REG_CURR as the write position, i.e. the position where the next packet is put. The SW
 * should move REG_BNRY forward if a packet has been read and can therefore be overwritten.
 * The range from 0x4000 to 0x5000 is used as a buffer for transmitting packets.
 */

Ne2k::Ne2k(esc::PCI &pci,const esc::PCI::Device &nic)
		: _irq(nic.irq), _irqsem(semcrtirq(_irq,"NE2000",NULL,NULL)), _basePort(), _mac(),
		  _nextPacket(), _handler() {
	print("Listening to IRQ %d",_irq);
	// create the IRQ sem here to ensure that we've registered it if the first interrupt arrives
	if(_irqsem < 0)
		error("Unable to create irq-semaphore");

	for(size_t i = 0; i < 6; i++) {
		if(nic.bars[i].addr && nic.bars[i].type == esc::PCI::Bar::BAR_IO) {
			print("Requesting ports %u..%u",nic.bars[i].addr,nic.bars[i].addr + nic.bars[i].size - 1);
			if(reqports(nic.bars[i].addr,nic.bars[i].size) < 0) {
				error("Unable to request io-ports %d..%d",
						nic.bars[i].addr,nic.bars[i].addr + nic.bars[i].size - 1);
			}
			_basePort = nic.bars[i].addr;
		}
	}

	print("Reset device");

	/* reset device */
	writeReg(REG_CMD,CMD_STP | CMD_COMPLDMA);
	/* enable 16-bit transfers and loopback */
	writeReg(REG_DCR,DCR_WTS | DCR_LS);
	/* enable monitor mode; we don't want to receive packets yet */
	writeReg(REG_RCR,RCR_MON);
	/* enable loopback control */
	writeReg(REG_TCR,TCR_LB0);

	print("Disable interrupts");

	/* disable interrupts */
	writeReg(REG_ISR,0xFF);	// clear bits by writing a 1
	writeReg(REG_IMR,0);	// mask all interrupts

	print("Read MAC address");

	/* read MAC address from PROM */
	uint16_t prom[16];
	accessPROM(0,32,prom,PROM_READ);
	for(size_t i = 0; i < 6; ++i)
		_mac[i] = prom[i] & 0xFF;

	print("Set MAC address");

	/* set the MAC address */
	writeReg(REG_CMD,CMD_STP | CMD_PAGE1 | CMD_COMPLDMA);
	for(size_t i = 0; i < 6; ++i)
		writeReg(REG_PAR0 + i,prom[i] & 0xff);

	print("Setup ringbuffer");

	/* setup packet ringbuffer */
	writeReg(REG_CMD,CMD_STP | CMD_PAGE1 | CMD_COMPLDMA);
	_nextPacket = PAGE_RX + 1;
	writeReg(REG_CURR,_nextPacket);

	/* back to page 0 */
	writeReg(REG_CMD,CMD_STP | CMD_COMPLDMA);
	/* configure start, boundary and stop */
	writeReg(REG_PSTART,PAGE_RX);
	writeReg(REG_BNRY,PAGE_RX);
	writeReg(REG_PSTOP,PAGE_STOP);

	/* accept broadcast and runt packets (< 64 bytes) */
	writeReg(REG_RCR,RCR_AR | RCR_AB);
	writeReg(REG_TCR,0);

	print("Enable interrupts");

	// ensure that interrupts are enabled for the PCI device
	uint32_t statusCmd = pci.read(nic.bus,nic.dev,nic.func,0x04);
	pci.write(nic.bus,nic.dev,nic.func,0x04,statusCmd & ~0x400);

	/* clear pending interrupts, enable them and begin card operation */
	writeReg(REG_ISR,0xFF);
	writeReg(REG_IMR,0x3F);
	writeReg(REG_CMD,CMD_STA | CMD_COMPLDMA);
}

void Ne2k::accessPROM(uint16_t offset,size_t size,void *buffer,Mode mode) {
	writeReg(REG_RSAR0,offset & 0xFF);
	writeReg(REG_RSAR1,offset >> 8);
	writeReg(REG_RBCR0,size & 0xFF);
	writeReg(REG_RBCR1,size >> 8);
	writeReg(REG_CMD,CMD_STA | (mode == PROM_READ ? CMD_READDMA : CMD_WRITEDMA));

	size_t i;
	uint16_t* words = (uint16_t*)buffer;
	if(mode == PROM_READ) {
		for(i = 0; i < size; i += 2)
			words[i / 2] = readReg16(REG_DATA);
		/* handle odd byte */
		//if(size & 1)
		//	words[i / 2] = readReg16(REG_DATA) & 0xFF;
	}
	else {
		for(i = 0; i < size; i += 2)
			writeReg16(REG_DATA,words[i / 2]);
		/* handle odd byte */
		//if(size & 1)
		//	writeReg(REG_DATA,words[i / 2]);
	}

	/* wait until the card is ready */
	while(!(readReg(REG_ISR) & ISR_RDC))
		;
	writeReg(REG_ISR,ISR_RDC);
}

ulong Ne2k::mtu() const {
	return (PAGE_RX - PAGE_TX) * PAGE_SZ;
}

ssize_t Ne2k::send(const void *packet,size_t size) {
	/* take care not to overwrite the receive buffer */
	if(size > mtu())
		return -ENOSPC;

	/* write data */
	accessPROM(PAGE_TX << 8,size,const_cast<void*>(packet),PROM_WRITE);

	/* execute the transmission */
	writeReg(REG_TBCR0,size > MIN_PKT_SIZE ? (size & 0xFF) : MIN_PKT_SIZE);
	writeReg(REG_TBCR1,size >> 8);

	writeReg(REG_TPSR,PAGE_TX);
	writeReg(REG_CMD,CMD_STA | CMD_TXP | CMD_COMPLDMA);
	return size;
}

void Ne2k::receive() {
	/* fetch current counter */
	writeReg(REG_CMD,CMD_COMPLDMA | CMD_PAGE1 | CMD_STP);
	uint8_t current = readReg(REG_CURR);
	writeReg(REG_CMD,CMD_COMPLDMA | CMD_STP);

	while(_nextPacket != current) {
		struct {
			uint16_t status;
			uint16_t length;
		} A_PACKED head;
		accessPROM(_nextPacket << 8,4,&head,PROM_READ);

		if(head.length == 0) {
			print("Received packet with zero-length");
			break;
		}

		head.length -= 4;

		/* read data into packet */
		Packet *pkt = (Packet*)malloc(sizeof(Packet) + head.length);
		if(!pkt) {
			printe("Not enough memory to read packet");
			break;
		}
		pkt->length = head.length;
		accessPROM((_nextPacket << 8) | 0x4,head.length,pkt->data,PROM_READ);

		/* move boundary forward */
		_nextPacket = head.status >> 8;
		writeReg(REG_BNRY,_nextPacket == PAGE_RX ? (PAGE_STOP - 1) : _nextPacket - 1);

		/* insert into list */
		insert(pkt);
		(*_handler)();
	}
}

int Ne2k::irqThread(void *ptr) {
	Ne2k *ne2k = reinterpret_cast<Ne2k*>(ptr);
	while(1) {
		semdown(ne2k->_irqsem);

		uint32_t isr;
		while((isr = ne2k->readReg(REG_ISR)) & (ISR_PRX | ISR_PTX | ISR_OVW | ISR_CNT)) {
			/* packet received */
			if(isr & ISR_PRX) {
				/* ack interrupt */
				ne2k->writeReg(REG_ISR,ISR_PRX);

				if(isr & ISR_RXE)
					print("Packet reception failed");
				else {
					ne2k->receive();
					/* unmask interrupts. TODO necessary? */
					ne2k->writeReg(REG_ISR,0x3F);
				}
			}

			/* packet transmitted */
			if(isr & ISR_PTX) {
				if(isr & ISR_TXE)
					print("Packet transmission failed");
				ne2k->writeReg(REG_ISR,ISR_PTX | ISR_TXE);
			}

			if(isr & ISR_OVW) {
				print("Receive buffer overflow");
				ne2k->writeReg(REG_ISR,ISR_OVW);
			}
			if(isr & ISR_CNT) {
				print("Counter overflow");
				ne2k->writeReg(REG_ISR,ISR_CNT);
			}
		}
	}
	return 0;
}
