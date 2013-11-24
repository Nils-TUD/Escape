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
#include <sys/arch/i586/idt.h>

/**
 * Our ISRs
 */
EXTERN_C void isr0();
EXTERN_C void isr1();
EXTERN_C void isr2();
EXTERN_C void isr3();
EXTERN_C void isr4();
EXTERN_C void isr5();
EXTERN_C void isr6();
EXTERN_C void isr7();
EXTERN_C void isr8();
EXTERN_C void isr9();
EXTERN_C void isr10();
EXTERN_C void isr11();
EXTERN_C void isr12();
EXTERN_C void isr13();
EXTERN_C void isr14();
EXTERN_C void isr15();
EXTERN_C void isr16();
EXTERN_C void isr17();
EXTERN_C void isr18();
EXTERN_C void isr19();
EXTERN_C void isr20();
EXTERN_C void isr21();
EXTERN_C void isr22();
EXTERN_C void isr23();
EXTERN_C void isr24();
EXTERN_C void isr25();
EXTERN_C void isr26();
EXTERN_C void isr27();
EXTERN_C void isr28();
EXTERN_C void isr29();
EXTERN_C void isr30();
EXTERN_C void isr31();
EXTERN_C void isr32();
EXTERN_C void isr33();
EXTERN_C void isr34();
EXTERN_C void isr35();
EXTERN_C void isr36();
EXTERN_C void isr37();
EXTERN_C void isr38();
EXTERN_C void isr39();
EXTERN_C void isr40();
EXTERN_C void isr41();
EXTERN_C void isr42();
EXTERN_C void isr43();
EXTERN_C void isr44();
EXTERN_C void isr45();
EXTERN_C void isr46();
EXTERN_C void isr47();
EXTERN_C void isr48();
EXTERN_C void isr49();
EXTERN_C void isr50();
EXTERN_C void isr51();
EXTERN_C void isr52();
EXTERN_C void isr53();
EXTERN_C void isr54();
EXTERN_C void isr55();
EXTERN_C void isr56();
EXTERN_C void isr57();
/* the handler for a other interrupts */
EXTERN_C void isrNull();

/* the interrupt descriptor table */
IDT::Entry IDT::idt[IDT_COUNT];

void IDT::init() {
	/* setup the idt-pointer */
	Pointer idtPtr;
	idtPtr.address = (uintptr_t)idt;
	idtPtr.size = sizeof(idt) - 1;

	/* setup the idt */

	/* exceptions */
	set(0,isr0,DPL_KERNEL);
	set(1,isr1,DPL_KERNEL);
	set(2,isr2,DPL_KERNEL);
	set(3,isr3,DPL_KERNEL);
	set(4,isr4,DPL_KERNEL);
	set(5,isr5,DPL_KERNEL);
	set(6,isr6,DPL_KERNEL);
	set(7,isr7,DPL_KERNEL);
	set(8,isr8,DPL_KERNEL);
	set(9,isr9,DPL_KERNEL);
	set(10,isr10,DPL_KERNEL);
	set(11,isr11,DPL_KERNEL);
	set(12,isr12,DPL_KERNEL);
	set(13,isr13,DPL_KERNEL);
	set(14,isr14,DPL_KERNEL);
	set(15,isr15,DPL_KERNEL);
	set(16,isr16,DPL_KERNEL);
	set(17,isr17,DPL_KERNEL);
	set(18,isr18,DPL_KERNEL);
	set(19,isr19,DPL_KERNEL);
	set(20,isr20,DPL_KERNEL);
	set(21,isr21,DPL_KERNEL);
	set(22,isr22,DPL_KERNEL);
	set(23,isr23,DPL_KERNEL);
	set(24,isr24,DPL_KERNEL);
	set(25,isr25,DPL_KERNEL);
	set(26,isr26,DPL_KERNEL);
	set(27,isr27,DPL_KERNEL);
	set(28,isr28,DPL_KERNEL);
	set(29,isr29,DPL_KERNEL);
	set(30,isr30,DPL_KERNEL);
	set(31,isr31,DPL_KERNEL);

	/* ISA interrupts */
	set(32,isr32,DPL_KERNEL);
	set(33,isr33,DPL_KERNEL);
	set(34,isr34,DPL_KERNEL);
	set(35,isr35,DPL_KERNEL);
	set(36,isr36,DPL_KERNEL);
	set(37,isr37,DPL_KERNEL);
	set(38,isr38,DPL_KERNEL);
	set(39,isr39,DPL_KERNEL);
	set(40,isr40,DPL_KERNEL);
	set(41,isr41,DPL_KERNEL);
	set(42,isr42,DPL_KERNEL);
	set(43,isr43,DPL_KERNEL);
	set(44,isr44,DPL_KERNEL);
	set(45,isr45,DPL_KERNEL);
	set(46,isr46,DPL_KERNEL);
	set(47,isr47,DPL_KERNEL);

	/* debug interrupt + ack-signal */
	set(48,isr48,DPL_USER);
	set(49,isr49,DPL_USER);

	/* LAPIC interrupts */
	set(50,isr50,DPL_KERNEL);
	set(51,isr51,DPL_KERNEL);
	set(52,isr52,DPL_KERNEL);
	set(53,isr53,DPL_KERNEL);
	set(54,isr54,DPL_KERNEL);
	set(55,isr55,DPL_KERNEL);
	set(56,isr56,DPL_KERNEL);
	set(57,isr57,DPL_KERNEL);

	/* all other interrupts */
	for(size_t i = 58; i < 256; i++)
		set(i,isrNull,DPL_KERNEL);

	/* now we can use our idt */
	load(&idtPtr);
}

void IDT::set(size_t number,isr_func handler,uint8_t dpl) {
	idt[number].fix = 0xE00;
	idt[number].dpl = dpl;
	idt[number].present = number != INTEL_RES1 && number != INTEL_RES2;
	idt[number].selector = CODE_SEL;
	idt[number].offsetHigh = ((uintptr_t)handler >> 16) & 0xFFFF;
	idt[number].offsetLow = (uintptr_t)handler & 0xFFFF;
}
