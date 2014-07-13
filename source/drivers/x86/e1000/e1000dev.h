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
#include <esc/proto/pci.h>
#include <esc/proto/nic.h>
#include <esc/ipc/nicdevice.h>
#include <functor.h>
#include <mutex>

#define DBG_LEVEL	1

#if DBG_LEVEL > 0
#	define DBG1(fmt...)		print(fmt)
#else
#	define DBG1(...)
#endif

#if DBG_LEVEL > 1
#	define DBG2(fmt...)		print(fmt)
#else
#	define DBG2(...)
#endif

class EEPROM;

class E1000 : public ipc::NICDriver {
	friend class EEPROM;

	enum {
		REG_CTRL			= 0x0,			/* device control register */
		REG_STATUS			= 0x8,			/* device status register */
		REG_EECD			= 0x10,			/* EEPROM control/data register */
		REG_EERD			= 0x14,			/* EEPROM read register */
		REG_VET				= 0x38,			/* VLAN ether type */

		REG_ICR				= 0xc0,			/* interrupt cause read register */
		REG_IMS				= 0xd0,			/* interrupt mask set/read register */
		REG_IMC				= 0xd8,			/* interrupt mask clear register */

		REG_RCTL			= 0x100,		/* receive control register */
		REG_TCTL			= 0x400,		/* transmit control register */

		REG_PBA				= 0x1000,		/* packet buffer allocation */
		REG_PBS				= 0x1008,		/* packet buffer size */

		REG_RDBAL			= 0x2800,		/* register descriptor base address low */
		REG_RDBAH			= 0x2804,		/* register descriptor base address high */
		REG_RDLEN			= 0x2808,		/* register descriptor length */
		REG_RDH				= 0x2810,		/* register descriptor head */
		REG_RDT				= 0x2818,		/* register descriptor tail */

		REG_RDTR			= 0x2820,		/* receive delay timer register */
		REG_RDCTL			= 0x2828,		/* transmit descriptor control */
		REG_RADV			= 0x282c,		/* receive absolute interrupt delay timer */

		REG_TDBAL			= 0x3800,		/* transmit descriptor base address low */
		REG_TDBAH			= 0x3804,		/* transmit descriptor base address high */
		REG_TDLEN			= 0x3808,		/* transmit descriptor length */
		REG_TDH				= 0x3810,		/* transmit descriptor head */
		REG_TDT				= 0x3818,		/* transmit descriptor tail */

		REG_TIDV			= 0x3820,		/* transmit interrupt delay value */
		REG_TDCTL			= 0x3828,		/* transmit descriptor control */
		REG_TADV			= 0x382c,		/* transmit absolute interrupt delay timer */

		REG_RAL				= 0x5400,		/* filtering: receive address low */
		REG_RAH				= 0x5404,		/* filtering: receive address high */
	};

	enum {
		STATUS_LU			= 1 << 1,		/* link up */
	};

	enum {
		CTL_LRST			= 1 << 3,		/* link reset */
		CTL_ASDE			= 1 << 5,		/* auto speed detection enable */
		CTL_SLU				= 1 << 6,		/* set link up */
		CTL_FRCSPD			= 1 << 11,		/* force speed */
		CTL_FRCDPLX			= 1 << 12,		/* force duplex */
		CTL_RESET			= 1 << 26,		/* 1 = device reset; self-clearing */
		CTL_PHY_RESET		= 1 << 31,		/* 1 = PHY reset */
	};

	enum {
		XDCTL_ENABLE		= 1 << 25,		/* queue enable */
	};

	enum {
		ICR_TXDW			= 1 << 0,		/* transmit descriptor done */
		ICR_LSC				= 1 << 2,		/* link status change */
		ICR_RXT0			= 1 << 7,		/* set when the receive timer expires */
		ICR_RXO				= 1 << 10,		/* receive overrun */
	};

	enum {
		RCTL_ENABLE			= 1 << 1,
		RCTL_UPE			= 1 << 3,		/* unicast promiscuous mode */
		RCTL_MPE			= 1 << 4,		/* multicast promiscuous */
		RCTL_BAM			= 1 << 15,		/* broadcasts accept mode */
		RCTL_BSIZE_256		= 0x11 << 16,	/* receive buffer size = 256 bytes (if RCTL_BSEX = 0) */
		RCTL_BSIZE_512		= 0x10 << 16,	/* receive buffer size = 512 bytes (if RCTL_BSEX = 0) */
		RCTL_BSIZE_1K		= 0x01 << 16,	/* receive buffer size = 1024 bytes (if RCTL_BSEX = 0) */
		RCTL_BSIZE_2K		= 0x00 << 16,	/* receive buffer size = 2048 bytes (if RCTL_BSEX = 0) */
		RCTL_BSIZE_MASK		= 0x11 << 16,	/* mask for buffer size */
		RCTL_BSEX_MASK		= 0x01 << 25,	/* mask for size extension */
		RCTL_SECRC			= 1 << 26,		/* strip CRC */
	};

	enum {
		TCTL_ENABLE			= 1 << 1,
		TCTL_PSP			= 1 << 3,		/* pad short packets */
		TCTL_COLL_TSH		= 0x0F << 4,	/* collision threshold; number of transmission attempts */
		TCTL_COLL_DIST		= 0x40 << 12,	/* collision distance; pad packets to X bytes; 64 here */
		TCTL_COLT_MASK		= 0xff << 4,
		TCTL_COLD_MASK		= 0x3ff << 12,
	};

	enum {
		RAH_VALID			= 1 << 31,		/* marks a receive address filter as valid */
	};

	enum {
		EEPROM_OFS_MAC		= 0x0,			/* offset of the MAC in EEPROM */
	};
	enum {
		EERD_START			= 1 << 0,		/* start command */
		EERD_DONE_SMALL		= 1 << 4,		/* read done (small EERD) */
		EERD_DONE_LARGE		= 1 << 1,		/* read done (large EERD) */
		EERD_SHIFT_SMALL	= 8,			/* address shift (small) */
		EERD_SHIFT_LARGE	= 2,			/* address shift (large) */
	};

	enum {
		TX_CMD_EOP			= 0x01,			/* end of packet */
		TX_CMD_IFCS			= 0x02,			/* insert FCS/CRC */
	};

	enum {
		RDS_DONE			= 1 << 0,		/* receive descriptor status; indicates that the HW has
											 * finished the descriptor */
	};

	static const size_t RX_BUF_COUNT	= 8;
	static const size_t TX_BUF_COUNT	= 8;
	static const size_t RX_BUF_SIZE		= 2048;
	static const size_t TX_BUF_SIZE		= 2048;

	struct TxDesc {
		uint64_t buffer;
		uint16_t length;
		uint8_t checksumOffset;
		uint8_t cmd;
		uint8_t status;
		uint8_t checksumStart;
		uint16_t : 16;
	} A_PACKED A_ALIGNED(4);

	 struct RxDesc {
		uint64_t buffer;
		uint16_t length;
		uint16_t checksum;
		uint8_t status;
		uint8_t error;
		uint16_t : 16;
	} A_PACKED A_ALIGNED(4);

	struct Buffers {
		RxDesc rxDescs[RX_BUF_COUNT] A_ALIGNED(16);
		TxDesc txDescs[TX_BUF_COUNT] A_ALIGNED(16);
		uint8_t rxBuf[RX_BUF_COUNT * RX_BUF_SIZE];
		uint8_t txBuf[TX_BUF_COUNT * TX_BUF_SIZE];
	};

public:
	explicit E1000(ipc::PCI &pci,const ipc::PCI::Device &nic);

	void setHandler(std::Functor<void> *handler) {
		_handler = handler;
	}

	virtual ipc::NIC::MAC mac() const {
		return _mac;
	}
	virtual ulong mtu() const {
		return TX_BUF_SIZE;
	}
	virtual ssize_t send(const void *packet,size_t size);

private:
	static int irqThread(void *ptr);

	void readEEPROM(uint8_t *dest,size_t len);
	ipc::NIC::MAC readMAC();
	void receive();

	void writeReg(uint16_t reg,uint32_t value) {
		DBG2("REG[%#04x] <- %#08x",reg,value);
		_mmio[reg / sizeof(uint32_t)] = value;
	}
	uint32_t readReg(uint16_t reg) {
		uint32_t val = _mmio[reg / sizeof(uint32_t)];
		DBG2("REG[%#04x] -> %#08x",reg,val);
		return val;
	}

	void reset();

	int _irq;
	int _irqsem;
	uint32_t _curRxBuf;
	uint32_t _curTxBuf;
	Buffers *_bufs;
	Buffers *_bufsPhys;
	volatile uint32_t *_mmio;
	ipc::NIC::MAC _mac;
	std::Functor<void> *_handler;
};
