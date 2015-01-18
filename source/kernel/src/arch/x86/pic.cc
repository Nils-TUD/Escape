/**
 * $Id: intrpt.c 1014 2011-08-15 17:37:10Z nasmussen $
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

#include <arch/x86/pic.h>
#include <arch/x86/ports.h>
#include <common.h>
#include <interrupts.h>

void PIC::init() {
	/* starts the initialization. we want to send a ICW4 */
	Ports::out<uint8_t>(PORT_MASTER_CMD,ICW1_INIT | ICW1_NEED_ICW4);
	Ports::out<uint8_t>(PORT_SLAVE_CMD,ICW1_INIT | ICW1_NEED_ICW4);
	/* remap the irqs to 0x20 and 0x28 */
	Ports::out<uint8_t>(PORT_MASTER_DATA,Interrupts::IRQ_MASTER_BASE);
	Ports::out<uint8_t>(PORT_SLAVE_DATA,Interrupts::IRQ_SLAVE_BASE);
	/* continue */
	Ports::out<uint8_t>(PORT_MASTER_DATA,4);
	Ports::out<uint8_t>(PORT_SLAVE_DATA,2);

	/* we want to use 8086 mode */
	Ports::out<uint8_t>(PORT_MASTER_DATA,ICW4_8086);
	Ports::out<uint8_t>(PORT_SLAVE_DATA,ICW4_8086);

	/* enable all interrupts (set masks to 0) */
	Ports::out<uint8_t>(PORT_MASTER_DATA,0x00);
	Ports::out<uint8_t>(PORT_SLAVE_DATA,0x00);
}
