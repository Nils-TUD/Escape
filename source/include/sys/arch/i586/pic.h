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
#include <sys/arch/i586/ports.h>
#include <sys/interrupts.h>

class PIC {
	PIC() = delete;

	/* I/O ports for PICs */
	enum {
		PORT_MASTER				= 0x20,					/* base-port for master PIC */
		PORT_SLAVE				= 0xA0,					/* base-port for slave PIC */
		PORT_MASTER_CMD			= PORT_MASTER,			/* command-port for master PIC */
		PORT_MASTER_DATA		= (PORT_MASTER + 1),	/* data-port for master PIC */
		PORT_SLAVE_CMD			= PORT_SLAVE,			/* command-port for slave PIC */
		PORT_SLAVE_DATA			= (PORT_SLAVE + 1),		/* data-port for slave PIC */
	};

	enum {
		EOI						= 0x20,					/* end of interrupt */
	};

	/* flags in Initialization Command Word 1 (ICW1) */
	enum {
		ICW1_NEED_ICW4			= 0x01,					/* ICW4 needed */
		ICW1_SINGLE				= 0x02,					/* Single (not cascade) mode */
		ICW1_INTERVAL4			= 0x04,					/* Call address interval 4 (instead of 8) */
		ICW1_LEVEL				= 0x08,					/* Level triggered (not edge) mode */
		ICW1_INIT				= 0x10,					/* Initialization - required! */
	};

	/* flags in Initialization Command Word 4 (ICW4) */
	enum {
		ICW4_8086				= 0x01,					/* 8086/88 (instead of MCS-80/85) mode */
		ICW4_AUTO				= 0x02,					/* Auto (instead of normal) EOI */
		ICW4_BUF_SLAVE			= 0x08,					/* Buffered mode/slave */
		ICW4_BUF_MASTER			= 0x0C,					/* Buffered mode/master */
		ICW4_SFNM				= 0x10,					/* Special fully nested */
	};

public:
	/**
	 * Inits the PIC
	 */
	static void init();

	/**
	 * Disables the PIC
	 */
	static void disable() {
		/* mask all interrupts */
		Ports::out<uint8_t>(PORT_MASTER_DATA,0xFF);
		Ports::out<uint8_t>(PORT_SLAVE_DATA,0xFF);
	}

	/**
	 * Masks the given interrupt
	 */
	static void mask(int irq) {
		uint port = PORT_MASTER_DATA;
		if(irq  >= Interrupts::IRQ_SLAVE_BASE) {
			port = PORT_SLAVE_DATA;
			irq -= 8;
		}
		uint8_t mask = Ports::in<uint8_t>(port);
		Ports::out<uint8_t>(port,mask | irq);
	}

	/**
	 * Sends an EOI for the given interrupt-number
	 */
	static void eoi(int intrptNo) {
		/* do we have to send EOI? */
		if(intrptNo >= Interrupts::IRQ_MASTER_BASE &&
				intrptNo <= Interrupts::IRQ_MASTER_BASE + Interrupts::IRQ_NUM) {
			if(intrptNo >= Interrupts::IRQ_SLAVE_BASE) {
				/* notify the slave */
				Ports::out<uint8_t>(PORT_SLAVE_CMD,EOI);
			}

			/* notify the master */
			Ports::out<uint8_t>(PORT_MASTER_CMD,EOI);
		}
	}
};
