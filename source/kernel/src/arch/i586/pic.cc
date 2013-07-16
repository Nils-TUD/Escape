/**
 * $Id: intrpt.c 1014 2011-08-15 17:37:10Z nasmussen $
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/arch/i586/pic.h>
#include <sys/arch/i586/ports.h>
#include <sys/arch/i586/intrpt.h>

/* I/O ports for PICs */
#define PIC_MASTER				0x20				/* base-port for master PIC */
#define PIC_SLAVE				0xA0				/* base-port for slave PIC */
#define PIC_MASTER_CMD			PIC_MASTER			/* command-port for master PIC */
#define PIC_MASTER_DATA			(PIC_MASTER + 1)	/* data-port for master PIC */
#define PIC_SLAVE_CMD			PIC_SLAVE			/* command-port for slave PIC */
#define PIC_SLAVE_DATA			(PIC_SLAVE + 1)		/* data-port for slave PIC */

#define PIC_EOI					0x20				/* end of interrupt */

/* flags in Initialization Command Word 1 (ICW1) */
#define ICW1_NEED_ICW4			0x01				/* ICW4 needed */
#define ICW1_SINGLE				0x02				/* Single (not cascade) mode */
#define ICW1_INTERVAL4			0x04				/* Call address interval 4 (instead of 8) */
#define ICW1_LEVEL				0x08				/* Level triggered (not edge) mode */
#define ICW1_INIT				0x10				/* Initialization - required! */

/* flags in Initialization Command Word 4 (ICW4) */
#define ICW4_8086				0x01				/* 8086/88 (instead of MCS-80/85) mode */
#define ICW4_AUTO				0x02				/* Auto (instead of normal) EOI */
#define ICW4_BUF_SLAVE			0x08				/* Buffered mode/slave */
#define ICW4_BUF_MASTER			0x0C				/* Buffered mode/master */
#define ICW4_SFNM				0x10				/* Special fully nested */

void pic_init(void) {
	/* starts the initialization. we want to send a ICW4 */
	ports_outByte(PIC_MASTER_CMD,ICW1_INIT | ICW1_NEED_ICW4);
	ports_outByte(PIC_SLAVE_CMD,ICW1_INIT | ICW1_NEED_ICW4);
	/* remap the irqs to 0x20 and 0x28 */
	ports_outByte(PIC_MASTER_DATA,IRQ_MASTER_BASE);
	ports_outByte(PIC_SLAVE_DATA,IRQ_SLAVE_BASE);
	/* continue */
	ports_outByte(PIC_MASTER_DATA,4);
	ports_outByte(PIC_SLAVE_DATA,2);

	/* we want to use 8086 mode */
	ports_outByte(PIC_MASTER_DATA,ICW4_8086);
	ports_outByte(PIC_SLAVE_DATA,ICW4_8086);

	/* enable all interrupts (set masks to 0) */
	ports_outByte(PIC_MASTER_DATA,0x00);
	ports_outByte(PIC_SLAVE_DATA,0x00);
}

void pic_eoi(uint32_t intrptNo) {
	/* do we have to send EOI? */
	if(intrptNo >= IRQ_MASTER_BASE && intrptNo <= IRQ_MASTER_BASE + IRQ_NUM) {
	    if(intrptNo >= IRQ_SLAVE_BASE) {
	    	/* notify the slave */
	        ports_outByte(PIC_SLAVE_CMD,PIC_EOI);
	    }

	    /* notify the master */
	    ports_outByte(PIC_MASTER_CMD,PIC_EOI);
    }
}
