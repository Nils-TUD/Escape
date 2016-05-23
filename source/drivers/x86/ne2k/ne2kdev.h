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

#include <sys/arch/x86/ports.h>
#include <esc/ipc/nicdevice.h>
#include <esc/proto/nic.h>
#include <esc/proto/pci.h>
#include <sys/common.h>
#include <sys/irq.h>
#include <sys/messages.h>
#include <sys/sync.h>
#include <sys/thread.h>
#include <functor.h>
#include <mutex>

class Ne2k : public esc::NICDriver {
	enum Mode {
		PROM_READ,
		PROM_WRITE
	};

	/* page 0 */
	enum {
		REG_CMD			= 0x00,		/* command register */
		REG_PSTART		= 0x01,		/* page start register */
		REG_PSTOP		= 0x02,		/* page stop register */
		REG_BNRY		= 0x03,		/* boundary pointer */
		REG_TPSR		= 0x04,		/* transmit page start register */
		REG_TBCR0		= 0x05,		/* transmit byte count register 0 */
		REG_TBCR1		= 0x06,		/* transmit byte count register 0 */
		REG_ISR			= 0x07,		/* interrupt status regsiter */
		REG_RSAR0		= 0x08,		/* remote start address register 0 */
		REG_RSAR1		= 0x09,		/* remote start address register 0 */
		REG_RBCR0		= 0x0A,		/* remote byte count register 0 */
		REG_RBCR1		= 0x0B,		/* remote byte count register 1 */
		REG_RCR			= 0x0C,		/* receive configuration register */
		REG_TCR			= 0x0D,		/* transmit configuration register */
		REG_DCR      	= 0x0E,		/* data configuration register */
		REG_IMR			= 0x0F,		/* interrupt mask register */
		REG_DATA		= 0x10,		/* remote DMA port */
	};

	/* page 1 */
	enum {
		REG_PAR0		= 0x01,		/* physical address register 0 (followed by 1,2,3,4 and 5) */
		REG_CURR		= 0x07,		/* current page */
	};

	enum {
		CMD_STP			= 1 << 0,	/* software reset */
		CMD_STA			= 1 << 1,	/* activate NIC */
		CMD_TXP			= 1 << 2,	/* transmit packet */
		CMD_READDMA		= 1 << 3,	/* remote dma;	rd2=0,rd1=0,rd0=1: remote read */
		CMD_WRITEDMA	= 1 << 4,	/* 				rd2=0,rd1=1,rd0=0: remote write */
		CMD_SENDPKT		= 3 << 3,	/* 				rd2=0,rd1=1,rd0=1: send packet */
		CMD_COMPLDMA	= 1 << 5,	/*				rd2=1,rd1=X,rd0=X: abort/complete remote DMA */
		CMD_PAGE1		= 1 << 6,	/* page select: ps1=0,ps0=0: register page 0 */
		CMD_PAGE2		= 1 << 7,	/*				ps1=0,ps1=1: register page 1 */
									/*				ps1=1,ps1=0: register page 2 */
	};

	enum {
		DCR_WTS			= 1 << 0,	/* word transfer select; 1 = word, 0 = byte */
		DCR_BOS			= 1 << 1,	/* byte order select; 0 = MSB in AD15-AD8, 1 = MSB in AD7-AD0 */
		DCR_LAS			= 1 << 2,	/* long address select; 0 = dual 16-bit DMA, 1 = single 32-bit */
		DCR_LS			= 1 << 3,	/* loopback select; 0 = loopback mode, 1 = normal operation */
	};

	enum {
		RCR_SEP			= 1 << 0,	/* save errored packets */
		RCR_AR			= 1 << 1,	/* accept runt packets (packets with less than 64 bytes) */
		RCR_AB			= 1 << 2,	/* accept broadcast (all 1's in the dest-address) */
		RCR_AM			= 1 << 3,	/* accept multicast */
		RCR_PRO			= 1 << 4,	/* promiscuous physical; phys-addr must match with PAR0-PAR5 */
		RCR_MON			= 1 << 5,	/* monitor mode; 0 = buffer to mem, 1 = don't buffer to mem */
	};

	enum {
		TCR_CRC			= 1 << 0,	/* inhibit CRC; 0 = CRC appended by transmitter, 1 = inhibited */
		TCR_LB0			= 1 << 1,	/* encoded loopback control */
		TCR_LB1			= 1 << 2,
		TCR_ATD			= 1 << 3,	/* auto transmit disable */
		TCR_OFST		= 1 << 4,	/* collision offset enable */
	};

	enum {
		ISR_PRX			= 1 << 0,	/* packet received */
		ISR_PTX			= 1 << 1,	/* packet transmitted */
		ISR_RXE			= 1 << 2,	/* receive error */
		ISR_TXE			= 1 << 3,	/* transmit error */
		ISR_OVW			= 1 << 4,	/* overwrite warning */
		ISR_CNT			= 1 << 5,	/* counter overflow */
		ISR_RDC			= 1 << 6,	/* remote DMA complete */
		ISR_RST			= 1 << 7,	/* reset status */
	};

	enum {
		PAGE_TX			= 0x40,		/* start page of the transmit buffer */
		PAGE_RX			= 0x50,		/* start page of the receive buffer ring */
		PAGE_STOP		= 0x80,		/* should not exceed 0x80 in 16-bit mode */
		PAGE_SZ			= 256,		/* bytes per page */
	};

	// TODO some OSs set that to 64
	static const size_t MIN_PKT_SIZE	= 60;

public:
	static const unsigned VENDOR_ID		= 0x10ec;
	static const unsigned DEVICE_ID		= 0x8029;

	explicit Ne2k(esc::PCI &pci,const esc::PCI::Device &nic);

	void start(std::Functor<void> *handler) {
		_handler = handler;
		if(startthread(irqThread,this) < 0)
			error("Unable to start receive-thread");
	}

	virtual esc::NIC::MAC mac() const {
		return esc::NIC::MAC(_mac);
	}
	virtual ulong mtu() const;
	virtual ssize_t send(const void *packet,size_t size);

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

	int _irq;
	int _irqsem;
	uint16_t _basePort;
	uint8_t _mac[6];
	uint8_t _nextPacket;
	std::Functor<void> *_handler;
};
