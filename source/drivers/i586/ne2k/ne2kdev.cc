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
#include <esc/thread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ne2kdev.h"

/* page 0 */
#define REG_CMD			0x00		/* command register */
#define REG_PSTART		0x01		/* page start register */
#define REG_PSTOP		0x02		/* page stop register */
#define REG_BNRY		0x03		/* boundary pointer */
#define REG_TPSR		0x04		/* transmit page start register */
#define REG_TBCR0		0x05		/* transmit byte count register 0 */
#define REG_TBCR1		0x06		/* transmit byte count register 0 */
#define REG_ISR			0x07		/* interrupt status regsiter */
#define REG_RSAR0		0x08		/* remote start address register 0 */
#define REG_RSAR1		0x09		/* remote start address register 0 */
#define REG_RBCR0		0x0A		/* remote byte count register 0 */
#define REG_RBCR1		0x0B		/* remote byte count register 1 */
#define REG_RCR			0x0C		/* receive configuration register */
#define REG_TCR			0x0D		/* transmit configuration register */
#define REG_DCR      	0x0E		/* data configuration register */
#define REG_IMR			0x0F		/* interrupt mask register */
#define REG_DATA		0x10		/* remote DMA port */

/* page 1 */
#define REG_PAR0		0x01		/* physical address register 0 (followed by 1,2,3,4 and 5) */
#define REG_CURR		0x07		/* current page */

#define CMD_STP			(1 << 0)	/* software reset */
#define CMD_STA			(1 << 1)	/* activate NIC */
#define CMD_TXP			(1 << 2)	/* transmit packet */
#define CMD_READDMA		(1 << 3)	/* remote dma;	rd2=0,rd1=0,rd0=1: remote read */
#define CMD_WRITEDMA	(1 << 4)	/* 				rd2=0,rd1=1,rd0=0: remote write */
#define CMD_SENDPKT		(3 << 3)	/* 				rd2=0,rd1=1,rd0=1: send packet */
#define CMD_COMPLDMA	(1 << 5)	/*				rd2=1,rd1=X,rd0=X: abort/complete remote DMA */
#define CMD_PAGE1		(1 << 6)	/* page select: ps1=0,ps0=0: register page 0 */
#define CMD_PAGE2		(1 << 7)	/*				ps1=0,ps1=1: register page 1 */
									/*				ps1=1,ps1=0: register page 2 */

#define DCR_WTS			(1 << 0)	/* word transfer select; 1 = word, 0 = byte */
#define DCR_BOS			(1 << 1)	/* byte order select; 0 = MSB in AD15-AD8, 1 = MSB in AD7-AD0 */
#define DCR_LAS			(1 << 2)	/* long address select; 0 = dual 16-bit DMA, 1 = single 32-bit */
#define DCR_LS			(1 << 3)	/* loopback select; 0 = loopback mode, 1 = normal operation */

#define RCR_SEP			(1 << 0)	/* save errored packets */
#define RCR_AR			(1 << 1)	/* accept runt packets (packets with less than 64 bytes) */
#define RCR_AB			(1 << 2)	/* accept broadcast (all 1's in the dest-address) */
#define RCR_AM			(1 << 3)	/* accept multicast */
#define RCR_PRO			(1 << 4)	/* promiscuous physical; phys-addr must match with PAR0-PAR5 */
#define RCR_MON			(1 << 5)	/* monitor mode; 0 = buffer to mem, 1 = don't buffer to mem */

#define TCR_CRC			(1 << 0)	/* inhibit CRC; 0 = CRC appended by transmitter, 1 = inhibited */
#define TCR_LB0			(1 << 1)	/* encoded loopback control */
#define TCR_LB1			(1 << 2)
#define TCR_ATD			(1 << 3)	/* auto transmit disable */
#define TCR_OFST		(1 << 4)	/* collision offset enable */

#define ISR_PRX			(1 << 0)	/* packet received */
#define ISR_PTX			(1 << 1)	/* packet transmitted */
#define ISR_RXE			(1 << 2)	/* receive error */
#define ISR_TXE			(1 << 3)	/* transmit error */
#define ISR_OVW			(1 << 4)	/* overwrite warning */
#define ISR_CNT			(1 << 5)	/* counter overflow */
#define ISR_RDC			(1 << 6)	/* remote DMA complete */
#define ISR_RST			(1 << 7)	/* reset status */

#define PAGE_TX			0x40		/* start page of the transmit buffer */
#define PAGE_RX			0x50		/* start page of the receive buffer ring */
#define PAGE_STOP		0x80		/* should not exceed 0x80 in 16-bit mode */
#define PAGE_SZ			256			/* bytes per page */

// TODO some OSs set that to 64
#define MIN_PKT_SIZE	60

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

Ne2k::Ne2k(const ipc::PCI::Device &nic,int sid)
		: _sid(sid), _basePort(), _mac(), _nextPacket(), _listmutex(), _first(), _last() {
	for(size_t i = 0; i < 6; i++) {
		if(nic.bars[i].addr && nic.bars[i].type == ipc::PCI::Bar::BAR_IO) {
			if(reqports(nic.bars[i].addr,nic.bars[i].size) < 0) {
				error("Unable to request io-ports %d..%d",
						nic.bars[i].addr,nic.bars[i].addr + nic.bars[i].size - 1);
			}
			_basePort = nic.bars[i].addr;
		}
	}

	/* reset device */
	writeReg(REG_CMD,CMD_STP | CMD_COMPLDMA);
	/* enable 16-bit transfers and loopback */
	writeReg(REG_DCR,DCR_WTS | DCR_LS);
	/* enable monitor mode; we don't want to receive packets yet */
	writeReg(REG_RCR,RCR_MON);
	/* enable loopback control */
	writeReg(REG_TCR,TCR_LB0);

	/* disable interrupts */
	writeReg(REG_ISR,0xFF);	// clear bits by writing a 1
	writeReg(REG_IMR,0);	// mask all interrupts

	/* read MAC address from PROM */
	uint16_t prom[16];
	accessPROM(0,32,prom,PROM_READ);
	for(size_t i = 0; i < 6; ++i)
		_mac[i] = prom[i] & 0xFF;

	/* set the MAC address */
	writeReg(REG_CMD,CMD_STP | CMD_PAGE1 | CMD_COMPLDMA);
	for(size_t i = 0; i < 6; ++i)
		writeReg(REG_PAR0 + i,prom[i] & 0xff);

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

	/* clear pending interrupts, enable them and begin card operation */
	writeReg(REG_ISR,0xFF);
	writeReg(REG_IMR,0x3F);
	writeReg(REG_CMD,CMD_STA | CMD_COMPLDMA);

	if(startthread(irqThread,this) < 0)
		error("Unable to start receive-thread");
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

ssize_t Ne2k::send(const void *packet,size_t size) {
	/* take care not to overwrite the receive buffer */
	if(size > (PAGE_RX - PAGE_TX) * PAGE_SZ)
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

ssize_t Ne2k::fetch(void *buffer,size_t size) {
	Packet *pkt = NULL;
	ssize_t res = 0;

	{
		std::lock_guard<std::mutex> guard(_listmutex);
		if(_first) {
			if(size < _first->length)
				return -EINVAL;
			pkt = _first;
			_first = _first->next;
			if(!_first)
				_last = NULL;
		}
	}

	if(pkt) {
		memcpy(buffer,pkt->data,pkt->length);
		res = pkt->length;
		free(pkt);
	}
	return res;
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
			printe("Received packet with zero-length");
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
		{
			std::lock_guard<std::mutex> guard(_listmutex);
			pkt->next = NULL;
			if(_last)
				_last->next = pkt;
			else
				_first = pkt;
			_last = pkt;
		}
		fcntl(_sid,F_WAKE_READER,0);
	}
}

int Ne2k::irqThread(void *ptr) {
	Ne2k *ne2k = reinterpret_cast<Ne2k*>(ptr);
	int irqsem = semcrtirq(IRQ_SEM_NE2K);
	if(irqsem < 0)
		error("Unable to create irq-semaphore");
	while(1) {
		semdown(irqsem);

		uint32_t isr = ne2k->readReg(REG_ISR);

		/* packet received */
		if(isr & ISR_PRX) {
			/* ack interrupt */
			ne2k->writeReg(REG_ISR,ISR_PRX);

			if(isr & ISR_RXE)
				printe("Packet reception failed");
			else {
				ne2k->receive();
				/* unmask interrupts. TODO necessary? */
				ne2k->writeReg(REG_ISR,0x3F);
			}
		}

		/* packet transmitted */
		if(isr & ISR_PTX) {
			if(isr & ISR_TXE)
				printe("Packet transmission failed");
			ne2k->writeReg(REG_ISR,ISR_PTX | ISR_TXE);
		}

		if(isr & ISR_OVW) {
			printe("Receive buffer overflow");
			ne2k->writeReg(REG_ISR,ISR_OVW);
		}
		if(isr & ISR_CNT) {
			printe("Counter overflow");
			ne2k->writeReg(REG_ISR,ISR_CNT);
		}
	}
	return 0;
}
