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

#define IDT_COUNT				256
/* the privilege level */
#define IDT_DPL_KERNEL			0
#define IDT_DPL_USER			3
/* reserved by intel */
#define IDT_INTEL_RES1			2
#define IDT_INTEL_RES2			15
/* the code-selector */
#define IDT_CODE_SEL			0x8

/* represents an IDT-entry */
typedef struct {
	/* The address[0..15] of the ISR */
	uint16_t offsetLow;
	/* Code selector that the ISR will use */
	uint16_t selector;
	/* these bits are fix: 0.1110.0000.0000b */
	uint16_t fix		: 13,
	/* the privilege level, 00 = ring0, 01 = ring1, 10 = ring2, 11 = ring3 */
	dpl			: 2,
	/* If Present is not set to 1, an exception will occur */
	present		: 1;
	/* The address[16..31] of the ISR */
	uint16_t	offsetHigh;
} A_PACKED sIDTEntry;

/* represents an IDT-pointer */
typedef struct {
	uint16_t size;
	uint32_t address;
} A_PACKED sIDTPtr;

/* isr prototype */
typedef void (*fISR)(void);

/**
 * Our ISRs
 */
EXTERN_C void isr0(void);
EXTERN_C void isr1(void);
EXTERN_C void isr2(void);
EXTERN_C void isr3(void);
EXTERN_C void isr4(void);
EXTERN_C void isr5(void);
EXTERN_C void isr6(void);
EXTERN_C void isr7(void);
EXTERN_C void isr8(void);
EXTERN_C void isr9(void);
EXTERN_C void isr10(void);
EXTERN_C void isr11(void);
EXTERN_C void isr12(void);
EXTERN_C void isr13(void);
EXTERN_C void isr14(void);
EXTERN_C void isr15(void);
EXTERN_C void isr16(void);
EXTERN_C void isr17(void);
EXTERN_C void isr18(void);
EXTERN_C void isr19(void);
EXTERN_C void isr20(void);
EXTERN_C void isr21(void);
EXTERN_C void isr22(void);
EXTERN_C void isr23(void);
EXTERN_C void isr24(void);
EXTERN_C void isr25(void);
EXTERN_C void isr26(void);
EXTERN_C void isr27(void);
EXTERN_C void isr28(void);
EXTERN_C void isr29(void);
EXTERN_C void isr30(void);
EXTERN_C void isr31(void);
EXTERN_C void isr32(void);
EXTERN_C void isr33(void);
EXTERN_C void isr34(void);
EXTERN_C void isr35(void);
EXTERN_C void isr36(void);
EXTERN_C void isr37(void);
EXTERN_C void isr38(void);
EXTERN_C void isr39(void);
EXTERN_C void isr40(void);
EXTERN_C void isr41(void);
EXTERN_C void isr42(void);
EXTERN_C void isr43(void);
EXTERN_C void isr44(void);
EXTERN_C void isr45(void);
EXTERN_C void isr46(void);
EXTERN_C void isr47(void);
EXTERN_C void isr48(void);
EXTERN_C void isr49(void);
EXTERN_C void isr50(void);
EXTERN_C void isr51(void);
EXTERN_C void isr52(void);
EXTERN_C void isr53(void);
EXTERN_C void isr54(void);
/* the handler for a other interrupts */
EXTERN_C void isrNull(void);

static void idt_set(size_t number,fISR handler,uint8_t dpl);

/* the interrupt descriptor table */
static sIDTEntry idt[IDT_COUNT];

void idt_init(void) {
	size_t i;

	/* setup the idt-pointer */
	sIDTPtr idtPtr;
	idtPtr.address = (uintptr_t)idt;
	idtPtr.size = sizeof(idt) - 1;

	/* setup the idt */

	/* exceptions */
	idt_set(0,isr0,IDT_DPL_KERNEL);
	idt_set(1,isr1,IDT_DPL_KERNEL);
	idt_set(2,isr2,IDT_DPL_KERNEL);
	idt_set(3,isr3,IDT_DPL_KERNEL);
	idt_set(4,isr4,IDT_DPL_KERNEL);
	idt_set(5,isr5,IDT_DPL_KERNEL);
	idt_set(6,isr6,IDT_DPL_KERNEL);
	idt_set(7,isr7,IDT_DPL_KERNEL);
	idt_set(8,isr8,IDT_DPL_KERNEL);
	idt_set(9,isr9,IDT_DPL_KERNEL);
	idt_set(10,isr10,IDT_DPL_KERNEL);
	idt_set(11,isr11,IDT_DPL_KERNEL);
	idt_set(12,isr12,IDT_DPL_KERNEL);
	idt_set(13,isr13,IDT_DPL_KERNEL);
	idt_set(14,isr14,IDT_DPL_KERNEL);
	idt_set(15,isr15,IDT_DPL_KERNEL);
	idt_set(16,isr16,IDT_DPL_KERNEL);
	idt_set(17,isr17,IDT_DPL_KERNEL);
	idt_set(18,isr18,IDT_DPL_KERNEL);
	idt_set(19,isr19,IDT_DPL_KERNEL);
	idt_set(20,isr20,IDT_DPL_KERNEL);
	idt_set(21,isr21,IDT_DPL_KERNEL);
	idt_set(22,isr22,IDT_DPL_KERNEL);
	idt_set(23,isr23,IDT_DPL_KERNEL);
	idt_set(24,isr24,IDT_DPL_KERNEL);
	idt_set(25,isr25,IDT_DPL_KERNEL);
	idt_set(26,isr26,IDT_DPL_KERNEL);
	idt_set(27,isr27,IDT_DPL_KERNEL);
	idt_set(28,isr28,IDT_DPL_KERNEL);
	idt_set(29,isr29,IDT_DPL_KERNEL);
	idt_set(30,isr30,IDT_DPL_KERNEL);
	idt_set(31,isr31,IDT_DPL_KERNEL);
	idt_set(32,isr32,IDT_DPL_KERNEL);

	/* hardware-interrupts */
	idt_set(33,isr33,IDT_DPL_KERNEL);
	idt_set(34,isr34,IDT_DPL_KERNEL);
	idt_set(35,isr35,IDT_DPL_KERNEL);
	idt_set(36,isr36,IDT_DPL_KERNEL);
	idt_set(37,isr37,IDT_DPL_KERNEL);
	idt_set(38,isr38,IDT_DPL_KERNEL);
	idt_set(39,isr39,IDT_DPL_KERNEL);
	idt_set(40,isr40,IDT_DPL_KERNEL);
	idt_set(41,isr41,IDT_DPL_KERNEL);
	idt_set(42,isr42,IDT_DPL_KERNEL);
	idt_set(43,isr43,IDT_DPL_KERNEL);
	idt_set(44,isr44,IDT_DPL_KERNEL);
	idt_set(45,isr45,IDT_DPL_KERNEL);
	idt_set(46,isr46,IDT_DPL_KERNEL);
	idt_set(47,isr47,IDT_DPL_KERNEL);

	/* syscall */
	idt_set(48,isr48,IDT_DPL_USER);
	/* IPIs */
	idt_set(49,isr49,IDT_DPL_KERNEL);
	idt_set(50,isr50,IDT_DPL_KERNEL);
	idt_set(51,isr51,IDT_DPL_KERNEL);
	idt_set(52,isr52,IDT_DPL_KERNEL);
	idt_set(53,isr53,IDT_DPL_KERNEL);
	idt_set(54,isr54,IDT_DPL_KERNEL);

	/* all other interrupts */
	for(i = 55; i < 256; i++)
		idt_set(i,isrNull,IDT_DPL_KERNEL);

	/* now we can use our idt */
	__asm__ volatile ("lidt %0" : : "m" (idtPtr));
}

static void idt_set(size_t number,fISR handler,uint8_t dpl) {
	idt[number].fix = 0xE00;
	idt[number].dpl = dpl;
	idt[number].present = number != IDT_INTEL_RES1 && number != IDT_INTEL_RES2;
	idt[number].selector = IDT_CODE_SEL;
	idt[number].offsetHigh = ((uintptr_t)handler >> 16) & 0xFFFF;
	idt[number].offsetLow = (uintptr_t)handler & 0xFFFF;
}
