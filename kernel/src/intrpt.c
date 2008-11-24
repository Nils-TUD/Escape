/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../pub/common.h"
#include "../pub/video.h"
#include "../pub/util.h"
#include "../pub/keyboard.h"
#include "../pub/cpu.h"
#include "../pub/paging.h"
#include "../pub/proc.h"
#include "../pub/elf.h"
#include "../pub/syscalls.h"

#include "../priv/intrpt.h"

/* interrupt -> name */
static cstring intrptNo2Name[] = {
	/* 0x00 */	"Divide by zero",
	/* 0x01 */	"Single step",
	/* 0x02 */	"Non maskable",
	/* 0x03 */	"Breakpoint",
	/* 0x04 */	"Overflow",
	/* 0x05 */	"Bounds check",
	/* 0x06 */	"Invalid opcode",
	/* 0x07 */	"Co-processor not available",
	/* 0x08 */	"Double fault",
	/* 0x09 */	"Co-processor segment overrun",
	/* 0x0A */	"Invalid TSS",
	/* 0x0B */	"Segment not present",
	/* 0x0C */	"Stack exception",
	/* 0x0D */	"General protection fault",
	/* 0x0E */	"Page fault",
	/* 0x0F */	"<unknown>",
	/* 0x10 */	"Co-processor error",
	/* 0x11 */	"<unknown>",
	/* 0x12 */	"<unknown>",
	/* 0x13 */	"<unknown>",
	/* 0x14 */	"<unknown>",
	/* 0x15 */	"<unknown>",
	/* 0x16 */	"<unknown>",
	/* 0x17 */	"<unknown>",
	/* 0x18 */	"<unknown>",
	/* 0x19 */	"<unknown>",
	/* 0x1A */	"<unknown>",
	/* 0x1B */	"<unknown>",
	/* 0x1C */	"<unknown>",
	/* 0x1D */	"<unknown>",
	/* 0x1E */	"<unknown>",
	/* 0x1F */	"<unknown>",
	/* 0x20 */	"Timer",
	/* 0x21 */	"Keyboard",
	/* 0x22 */	"<Cascade>",
	/* 0x23 */	"COM2",
	/* 0x24 */	"COM1",
	/* 0x25 */	"<unknown>",
	/* 0x26 */	"Floppy",
	/* 0x27 */	"<unknown>",
	/* 0x28 */	"CMOS real-time-clock",
	/* 0x29 */	"<unknown>",
	/* 0x2A */	"<unknown>",
	/* 0x2B */	"<unknown>",
	/* 0x2C */	"<unknown>",
	/* 0x2D */	"<unknown>",
	/* 0x2E */	"ATA1",
	/* 0x2F */	"ATA2"
};

/* stuff to count exceptions */
static u32 exCount = 0;
static u32 lastEx = 0xFFFFFFFF;

/**
 * An assembler routine to load an IDT
 *
 * @param idt the IDT to load
 */
/*extern void idt_flush(sIDTPtr *idt);*/

/* the IDT */
static sIDTEntry idt[IDT_COUNT];

cstring intrpt_no2Name(u32 intrptNo) {
	if(intrptNo < ARRAY_SIZE(intrptNo2Name)) {
		return intrptNo2Name[intrptNo];
	}

	return "Unknown interrupt";
}

void intrpt_init(void) {
	/* setup the idt-pointer */
	sIDTPtr idtPtr;
	idtPtr.address = (u32)idt;
	idtPtr.size = sizeof(idt) - 1;

	/* setup the idt */
	intrpt_setIDT(0,isr0,IDT_DPL_KERNEL);
	intrpt_setIDT(1,isr1,IDT_DPL_KERNEL);
	intrpt_setIDT(2,isr2,IDT_DPL_KERNEL);
	intrpt_setIDT(3,isr3,IDT_DPL_KERNEL);
	intrpt_setIDT(4,isr4,IDT_DPL_KERNEL);
	intrpt_setIDT(5,isr5,IDT_DPL_KERNEL);
	intrpt_setIDT(6,isr6,IDT_DPL_KERNEL);
	intrpt_setIDT(7,isr7,IDT_DPL_KERNEL);
	intrpt_setIDT(8,isr8,IDT_DPL_KERNEL);
	intrpt_setIDT(9,isr9,IDT_DPL_KERNEL);
	intrpt_setIDT(10,isr10,IDT_DPL_KERNEL);
	intrpt_setIDT(11,isr11,IDT_DPL_KERNEL);
	intrpt_setIDT(12,isr12,IDT_DPL_KERNEL);
	intrpt_setIDT(13,isr13,IDT_DPL_KERNEL);
	intrpt_setIDT(14,isr14,IDT_DPL_KERNEL);
	intrpt_setIDT(15,isr15,IDT_DPL_KERNEL);
	intrpt_setIDT(16,isr16,IDT_DPL_KERNEL);
	intrpt_setIDT(17,isr17,IDT_DPL_KERNEL);
	intrpt_setIDT(18,isr18,IDT_DPL_KERNEL);
	intrpt_setIDT(19,isr19,IDT_DPL_KERNEL);
	intrpt_setIDT(20,isr20,IDT_DPL_KERNEL);
	intrpt_setIDT(21,isr21,IDT_DPL_KERNEL);
	intrpt_setIDT(22,isr22,IDT_DPL_KERNEL);
	intrpt_setIDT(23,isr23,IDT_DPL_KERNEL);
	intrpt_setIDT(24,isr24,IDT_DPL_KERNEL);
	intrpt_setIDT(25,isr25,IDT_DPL_KERNEL);
	intrpt_setIDT(26,isr26,IDT_DPL_KERNEL);
	intrpt_setIDT(27,isr27,IDT_DPL_KERNEL);
	intrpt_setIDT(28,isr28,IDT_DPL_KERNEL);
	intrpt_setIDT(29,isr29,IDT_DPL_KERNEL);
	intrpt_setIDT(30,isr30,IDT_DPL_KERNEL);
	intrpt_setIDT(31,isr31,IDT_DPL_KERNEL);
	intrpt_setIDT(32,isr32,IDT_DPL_KERNEL);
	intrpt_setIDT(33,isr33,IDT_DPL_KERNEL);
	intrpt_setIDT(34,isr34,IDT_DPL_KERNEL);
	intrpt_setIDT(35,isr35,IDT_DPL_KERNEL);
	intrpt_setIDT(36,isr36,IDT_DPL_KERNEL);
	intrpt_setIDT(37,isr37,IDT_DPL_KERNEL);
	intrpt_setIDT(38,isr38,IDT_DPL_KERNEL);
	intrpt_setIDT(39,isr39,IDT_DPL_KERNEL);
	intrpt_setIDT(40,isr40,IDT_DPL_KERNEL);
	intrpt_setIDT(41,isr41,IDT_DPL_KERNEL);
	intrpt_setIDT(42,isr42,IDT_DPL_KERNEL);
	intrpt_setIDT(43,isr43,IDT_DPL_KERNEL);
	intrpt_setIDT(44,isr44,IDT_DPL_KERNEL);
	intrpt_setIDT(45,isr45,IDT_DPL_KERNEL);
	intrpt_setIDT(46,isr46,IDT_DPL_KERNEL);
	intrpt_setIDT(47,isr47,IDT_DPL_KERNEL);
	intrpt_setIDT(48,isr48,IDT_DPL_USER);	/* syscall */
	intrpt_setIDT(49,isr49,IDT_DPL_KERNEL);
	intrpt_setIDT(50,isr50,IDT_DPL_KERNEL);
	intrpt_setIDT(51,isr51,IDT_DPL_KERNEL);
	intrpt_setIDT(52,isr52,IDT_DPL_KERNEL);
	intrpt_setIDT(53,isr53,IDT_DPL_KERNEL);
	intrpt_setIDT(54,isr54,IDT_DPL_KERNEL);
	intrpt_setIDT(55,isr55,IDT_DPL_KERNEL);
	intrpt_setIDT(56,isr56,IDT_DPL_KERNEL);
	intrpt_setIDT(57,isr57,IDT_DPL_KERNEL);
	intrpt_setIDT(58,isr58,IDT_DPL_KERNEL);
	intrpt_setIDT(59,isr59,IDT_DPL_KERNEL);
	intrpt_setIDT(60,isr60,IDT_DPL_KERNEL);
	intrpt_setIDT(61,isr61,IDT_DPL_KERNEL);
	intrpt_setIDT(62,isr62,IDT_DPL_KERNEL);
	intrpt_setIDT(63,isr63,IDT_DPL_KERNEL);
	intrpt_setIDT(64,isr64,IDT_DPL_KERNEL);
	intrpt_setIDT(65,isr65,IDT_DPL_KERNEL);
	intrpt_setIDT(66,isr66,IDT_DPL_KERNEL);
	intrpt_setIDT(67,isr67,IDT_DPL_KERNEL);
	intrpt_setIDT(68,isr68,IDT_DPL_KERNEL);
	intrpt_setIDT(69,isr69,IDT_DPL_KERNEL);
	intrpt_setIDT(70,isr70,IDT_DPL_KERNEL);
	intrpt_setIDT(71,isr71,IDT_DPL_KERNEL);
	intrpt_setIDT(72,isr72,IDT_DPL_KERNEL);
	intrpt_setIDT(73,isr73,IDT_DPL_KERNEL);
	intrpt_setIDT(74,isr74,IDT_DPL_KERNEL);
	intrpt_setIDT(75,isr75,IDT_DPL_KERNEL);
	intrpt_setIDT(76,isr76,IDT_DPL_KERNEL);
	intrpt_setIDT(77,isr77,IDT_DPL_KERNEL);
	intrpt_setIDT(78,isr78,IDT_DPL_KERNEL);
	intrpt_setIDT(79,isr79,IDT_DPL_KERNEL);
	intrpt_setIDT(80,isr80,IDT_DPL_KERNEL);
	intrpt_setIDT(81,isr81,IDT_DPL_KERNEL);
	intrpt_setIDT(82,isr82,IDT_DPL_KERNEL);
	intrpt_setIDT(83,isr83,IDT_DPL_KERNEL);
	intrpt_setIDT(84,isr84,IDT_DPL_KERNEL);
	intrpt_setIDT(85,isr85,IDT_DPL_KERNEL);
	intrpt_setIDT(86,isr86,IDT_DPL_KERNEL);
	intrpt_setIDT(87,isr87,IDT_DPL_KERNEL);
	intrpt_setIDT(88,isr88,IDT_DPL_KERNEL);
	intrpt_setIDT(89,isr89,IDT_DPL_KERNEL);
	intrpt_setIDT(90,isr90,IDT_DPL_KERNEL);
	intrpt_setIDT(91,isr91,IDT_DPL_KERNEL);
	intrpt_setIDT(92,isr92,IDT_DPL_KERNEL);
	intrpt_setIDT(93,isr93,IDT_DPL_KERNEL);
	intrpt_setIDT(94,isr94,IDT_DPL_KERNEL);
	intrpt_setIDT(95,isr95,IDT_DPL_KERNEL);
	intrpt_setIDT(96,isr96,IDT_DPL_KERNEL);
	intrpt_setIDT(97,isr97,IDT_DPL_KERNEL);
	intrpt_setIDT(98,isr98,IDT_DPL_KERNEL);
	intrpt_setIDT(99,isr99,IDT_DPL_KERNEL);
	intrpt_setIDT(100,isr100,IDT_DPL_KERNEL);
	intrpt_setIDT(101,isr101,IDT_DPL_KERNEL);
	intrpt_setIDT(102,isr102,IDT_DPL_KERNEL);
	intrpt_setIDT(103,isr103,IDT_DPL_KERNEL);
	intrpt_setIDT(104,isr104,IDT_DPL_KERNEL);
	intrpt_setIDT(105,isr105,IDT_DPL_KERNEL);
	intrpt_setIDT(106,isr106,IDT_DPL_KERNEL);
	intrpt_setIDT(107,isr107,IDT_DPL_KERNEL);
	intrpt_setIDT(108,isr108,IDT_DPL_KERNEL);
	intrpt_setIDT(109,isr109,IDT_DPL_KERNEL);
	intrpt_setIDT(110,isr110,IDT_DPL_KERNEL);
	intrpt_setIDT(111,isr111,IDT_DPL_KERNEL);
	intrpt_setIDT(112,isr112,IDT_DPL_KERNEL);
	intrpt_setIDT(113,isr113,IDT_DPL_KERNEL);
	intrpt_setIDT(114,isr114,IDT_DPL_KERNEL);
	intrpt_setIDT(115,isr115,IDT_DPL_KERNEL);
	intrpt_setIDT(116,isr116,IDT_DPL_KERNEL);
	intrpt_setIDT(117,isr117,IDT_DPL_KERNEL);
	intrpt_setIDT(118,isr118,IDT_DPL_KERNEL);
	intrpt_setIDT(119,isr119,IDT_DPL_KERNEL);
	intrpt_setIDT(120,isr120,IDT_DPL_KERNEL);
	intrpt_setIDT(121,isr121,IDT_DPL_KERNEL);
	intrpt_setIDT(122,isr122,IDT_DPL_KERNEL);
	intrpt_setIDT(123,isr123,IDT_DPL_KERNEL);
	intrpt_setIDT(124,isr124,IDT_DPL_KERNEL);
	intrpt_setIDT(125,isr125,IDT_DPL_KERNEL);
	intrpt_setIDT(126,isr126,IDT_DPL_KERNEL);
	intrpt_setIDT(127,isr127,IDT_DPL_KERNEL);
	intrpt_setIDT(128,isr128,IDT_DPL_KERNEL);
	intrpt_setIDT(129,isr129,IDT_DPL_KERNEL);
	intrpt_setIDT(130,isr130,IDT_DPL_KERNEL);
	intrpt_setIDT(131,isr131,IDT_DPL_KERNEL);
	intrpt_setIDT(132,isr132,IDT_DPL_KERNEL);
	intrpt_setIDT(133,isr133,IDT_DPL_KERNEL);
	intrpt_setIDT(134,isr134,IDT_DPL_KERNEL);
	intrpt_setIDT(135,isr135,IDT_DPL_KERNEL);
	intrpt_setIDT(136,isr136,IDT_DPL_KERNEL);
	intrpt_setIDT(137,isr137,IDT_DPL_KERNEL);
	intrpt_setIDT(138,isr138,IDT_DPL_KERNEL);
	intrpt_setIDT(139,isr139,IDT_DPL_KERNEL);
	intrpt_setIDT(140,isr140,IDT_DPL_KERNEL);
	intrpt_setIDT(141,isr141,IDT_DPL_KERNEL);
	intrpt_setIDT(142,isr142,IDT_DPL_KERNEL);
	intrpt_setIDT(143,isr143,IDT_DPL_KERNEL);
	intrpt_setIDT(144,isr144,IDT_DPL_KERNEL);
	intrpt_setIDT(145,isr145,IDT_DPL_KERNEL);
	intrpt_setIDT(146,isr146,IDT_DPL_KERNEL);
	intrpt_setIDT(147,isr147,IDT_DPL_KERNEL);
	intrpt_setIDT(148,isr148,IDT_DPL_KERNEL);
	intrpt_setIDT(149,isr149,IDT_DPL_KERNEL);
	intrpt_setIDT(150,isr150,IDT_DPL_KERNEL);
	intrpt_setIDT(151,isr151,IDT_DPL_KERNEL);
	intrpt_setIDT(152,isr152,IDT_DPL_KERNEL);
	intrpt_setIDT(153,isr153,IDT_DPL_KERNEL);
	intrpt_setIDT(154,isr154,IDT_DPL_KERNEL);
	intrpt_setIDT(155,isr155,IDT_DPL_KERNEL);
	intrpt_setIDT(156,isr156,IDT_DPL_KERNEL);
	intrpt_setIDT(157,isr157,IDT_DPL_KERNEL);
	intrpt_setIDT(158,isr158,IDT_DPL_KERNEL);
	intrpt_setIDT(159,isr159,IDT_DPL_KERNEL);
	intrpt_setIDT(160,isr160,IDT_DPL_KERNEL);
	intrpt_setIDT(161,isr161,IDT_DPL_KERNEL);
	intrpt_setIDT(162,isr162,IDT_DPL_KERNEL);
	intrpt_setIDT(163,isr163,IDT_DPL_KERNEL);
	intrpt_setIDT(164,isr164,IDT_DPL_KERNEL);
	intrpt_setIDT(165,isr165,IDT_DPL_KERNEL);
	intrpt_setIDT(166,isr166,IDT_DPL_KERNEL);
	intrpt_setIDT(167,isr167,IDT_DPL_KERNEL);
	intrpt_setIDT(168,isr168,IDT_DPL_KERNEL);
	intrpt_setIDT(169,isr169,IDT_DPL_KERNEL);
	intrpt_setIDT(170,isr170,IDT_DPL_KERNEL);
	intrpt_setIDT(171,isr171,IDT_DPL_KERNEL);
	intrpt_setIDT(172,isr172,IDT_DPL_KERNEL);
	intrpt_setIDT(173,isr173,IDT_DPL_KERNEL);
	intrpt_setIDT(174,isr174,IDT_DPL_KERNEL);
	intrpt_setIDT(175,isr175,IDT_DPL_KERNEL);
	intrpt_setIDT(176,isr176,IDT_DPL_KERNEL);
	intrpt_setIDT(177,isr177,IDT_DPL_KERNEL);
	intrpt_setIDT(178,isr178,IDT_DPL_KERNEL);
	intrpt_setIDT(179,isr179,IDT_DPL_KERNEL);
	intrpt_setIDT(180,isr180,IDT_DPL_KERNEL);
	intrpt_setIDT(181,isr181,IDT_DPL_KERNEL);
	intrpt_setIDT(182,isr182,IDT_DPL_KERNEL);
	intrpt_setIDT(183,isr183,IDT_DPL_KERNEL);
	intrpt_setIDT(184,isr184,IDT_DPL_KERNEL);
	intrpt_setIDT(185,isr185,IDT_DPL_KERNEL);
	intrpt_setIDT(186,isr186,IDT_DPL_KERNEL);
	intrpt_setIDT(187,isr187,IDT_DPL_KERNEL);
	intrpt_setIDT(188,isr188,IDT_DPL_KERNEL);
	intrpt_setIDT(189,isr189,IDT_DPL_KERNEL);
	intrpt_setIDT(190,isr190,IDT_DPL_KERNEL);
	intrpt_setIDT(191,isr191,IDT_DPL_KERNEL);
	intrpt_setIDT(192,isr192,IDT_DPL_KERNEL);
	intrpt_setIDT(193,isr193,IDT_DPL_KERNEL);
	intrpt_setIDT(194,isr194,IDT_DPL_KERNEL);
	intrpt_setIDT(195,isr195,IDT_DPL_KERNEL);
	intrpt_setIDT(196,isr196,IDT_DPL_KERNEL);
	intrpt_setIDT(197,isr197,IDT_DPL_KERNEL);
	intrpt_setIDT(198,isr198,IDT_DPL_KERNEL);
	intrpt_setIDT(199,isr199,IDT_DPL_KERNEL);
	intrpt_setIDT(200,isr200,IDT_DPL_KERNEL);
	intrpt_setIDT(201,isr201,IDT_DPL_KERNEL);
	intrpt_setIDT(202,isr202,IDT_DPL_KERNEL);
	intrpt_setIDT(203,isr203,IDT_DPL_KERNEL);
	intrpt_setIDT(204,isr204,IDT_DPL_KERNEL);
	intrpt_setIDT(205,isr205,IDT_DPL_KERNEL);
	intrpt_setIDT(206,isr206,IDT_DPL_KERNEL);
	intrpt_setIDT(207,isr207,IDT_DPL_KERNEL);
	intrpt_setIDT(208,isr208,IDT_DPL_KERNEL);
	intrpt_setIDT(209,isr209,IDT_DPL_KERNEL);
	intrpt_setIDT(210,isr210,IDT_DPL_KERNEL);
	intrpt_setIDT(211,isr211,IDT_DPL_KERNEL);
	intrpt_setIDT(212,isr212,IDT_DPL_KERNEL);
	intrpt_setIDT(213,isr213,IDT_DPL_KERNEL);
	intrpt_setIDT(214,isr214,IDT_DPL_KERNEL);
	intrpt_setIDT(215,isr215,IDT_DPL_KERNEL);
	intrpt_setIDT(216,isr216,IDT_DPL_KERNEL);
	intrpt_setIDT(217,isr217,IDT_DPL_KERNEL);
	intrpt_setIDT(218,isr218,IDT_DPL_KERNEL);
	intrpt_setIDT(219,isr219,IDT_DPL_KERNEL);
	intrpt_setIDT(220,isr220,IDT_DPL_KERNEL);
	intrpt_setIDT(221,isr221,IDT_DPL_KERNEL);
	intrpt_setIDT(222,isr222,IDT_DPL_KERNEL);
	intrpt_setIDT(223,isr223,IDT_DPL_KERNEL);
	intrpt_setIDT(224,isr224,IDT_DPL_KERNEL);
	intrpt_setIDT(225,isr225,IDT_DPL_KERNEL);
	intrpt_setIDT(226,isr226,IDT_DPL_KERNEL);
	intrpt_setIDT(227,isr227,IDT_DPL_KERNEL);
	intrpt_setIDT(228,isr228,IDT_DPL_KERNEL);
	intrpt_setIDT(229,isr229,IDT_DPL_KERNEL);
	intrpt_setIDT(230,isr230,IDT_DPL_KERNEL);
	intrpt_setIDT(231,isr231,IDT_DPL_KERNEL);
	intrpt_setIDT(232,isr232,IDT_DPL_KERNEL);
	intrpt_setIDT(233,isr233,IDT_DPL_KERNEL);
	intrpt_setIDT(234,isr234,IDT_DPL_KERNEL);
	intrpt_setIDT(235,isr235,IDT_DPL_KERNEL);
	intrpt_setIDT(236,isr236,IDT_DPL_KERNEL);
	intrpt_setIDT(237,isr237,IDT_DPL_KERNEL);
	intrpt_setIDT(238,isr238,IDT_DPL_KERNEL);
	intrpt_setIDT(239,isr239,IDT_DPL_KERNEL);
	intrpt_setIDT(240,isr240,IDT_DPL_KERNEL);
	intrpt_setIDT(241,isr241,IDT_DPL_KERNEL);
	intrpt_setIDT(242,isr242,IDT_DPL_KERNEL);
	intrpt_setIDT(243,isr243,IDT_DPL_KERNEL);
	intrpt_setIDT(244,isr244,IDT_DPL_KERNEL);
	intrpt_setIDT(245,isr245,IDT_DPL_KERNEL);
	intrpt_setIDT(246,isr246,IDT_DPL_KERNEL);
	intrpt_setIDT(247,isr247,IDT_DPL_KERNEL);
	intrpt_setIDT(248,isr248,IDT_DPL_KERNEL);
	intrpt_setIDT(249,isr249,IDT_DPL_KERNEL);
	intrpt_setIDT(250,isr250,IDT_DPL_KERNEL);
	intrpt_setIDT(251,isr251,IDT_DPL_KERNEL);
	intrpt_setIDT(252,isr252,IDT_DPL_KERNEL);
	intrpt_setIDT(253,isr253,IDT_DPL_KERNEL);
	intrpt_setIDT(254,isr254,IDT_DPL_KERNEL);
	intrpt_setIDT(255,isr255,IDT_DPL_KERNEL);

	/* now we can use our idt */
	intrpt_loadidt(&idtPtr);

	/* now init the PIC */
	intrpt_initPic();
}

/* TODO temporary */
static u8 task2[] = {
	#include "../../build/user_task2.dump"
};
static bool proc2Ready = false;

void intrpt_handler(sIntrptStackFrame stack) {
	sProc *p;
	switch(stack.intrptNo) {
		case IRQ_KEYBOARD:
			kbd_handleIntrpt();
			break;

		case IRQ_TIMER:
			/* TODO don't resched if we come from kernel-mode! */
			if(stack.ds != 0x23)
				panic("Timer interrupt from kernel-mode!");

#if 0
			/*vid_printf("Timer interrupt...\n");*/
			if(!proc2Ready) {
				proc2Ready = true;
				/* clone proc1 */
				tPid pid = proc_getFreePid();
				if(proc_clone(pid)) {
					p = proc_getRunning();
					/* overwrite pages (copyonwrite is enabled) */
					paging_map(p->textPages * PAGE_SIZE,NULL,p->dataPages,PG_WRITABLE,true);
					paging_map(KERNEL_V_ADDR - p->stackPages * PAGE_SIZE,NULL,p->stackPages,
							PG_WRITABLE,true);
					/* now load task2 */
					vid_printf("Loading process %d\n",p->pid);
					elf_loadprog(task2);
					vid_printf("Starting...\n");
					proc_setupIntrptStack(&stack);
				}
#if 0
				else if(proc_getRunning()->pid == 0 && proc_clone(proc_getFreePid())) {
					p = proc_getRunning();
					vid_printf("Starting process %d\n",p->pid);
					proc_setupIntrptStack(&stack);
				}
#endif
				break;
			}

#endif
			intrpt_eoi(stack.intrptNo);
			proc_switch();
			break;

		/* syscall */
		case IRQ_SYSCALL:
			sysc_handle((sSysCallStack*)stack.uesp);
			break;

		/* exceptions */
		case EX_DIVIDE_BY_ZERO ... EX_CO_PROC_ERROR:
			/* #PF */
			if(stack.intrptNo == EX_PAGE_FAULT) {
				vid_printf("Page fault for address=0x%08x @ 0x%x\n",cpu_getCR2(),stack.eip);
				paging_handlePageFault(cpu_getCR2());
				break;
			}

			/* count consecutive occurrences */
			/* TODO we should consider irqs, too! */
			if(lastEx == stack.intrptNo) {
				exCount++;

				/* stop here? */
				if(exCount >= MAX_EX_COUNT)
					panic("Got this exception %d times. Stopping here\n",exCount);
			}
			else {
				exCount = 0;
				lastEx = stack.intrptNo;
			}

			/* #GPF */
			if(stack.intrptNo == EX_GEN_PROT_FAULT) {
				vid_printf("GPF for address=0x%08x @ 0x%x\n",cpu_getCR2(),stack.eip);
				printStackTrace();
				break;
			}
			/* fall through */

		default:
			vid_printf("Got interrupt %d (%s) @ 0x%x\n",stack.intrptNo,
					intrpt_no2Name(stack.intrptNo),stack.eip);
			break;
	}

	/* send EOI to PIC */
	intrpt_eoi(stack.intrptNo);
}

static void intrpt_initPic(void) {
	/* starts the initialization. we want to send a ICW4 */
	outb(PIC_MASTER_CMD,ICW1_INIT | ICW1_NEED_ICW4);
	outb(PIC_SLAVE_CMD,ICW1_INIT | ICW1_NEED_ICW4);
	/* remap the irqs to 0x20 and 0x28 */
	outb(PIC_MASTER_DATA,IRQ_MASTER_BASE);
	outb(PIC_SLAVE_DATA,IRQ_SLAVE_BASE);
	/* continue */
	outb(PIC_MASTER_DATA,4);
	outb(PIC_SLAVE_DATA,2);

	/* we want to use 8086 mode */
	outb(PIC_MASTER_DATA,ICW4_8086);
	outb(PIC_SLAVE_DATA,ICW4_8086);

	/* enable all interrupts (set masks to 0) */
	outb(PIC_MASTER_DATA,0x00);
	outb(PIC_SLAVE_DATA,0x00);
}

static void intrpt_setIDT(u16 number,fISR handler,u8 dpl) {
	idt[number].fix = 0xE00;
	idt[number].dpl = dpl;
	idt[number].present = number != IDT_INTEL_RES1 && number != IDT_INTEL_RES2;
	idt[number].selector = IDT_CODE_SEL;
	idt[number].offsetHigh = ((u32)handler >> 16) & 0xFFFF;
	idt[number].offsetLow = (u32)handler & 0xFFFF;
}

static void intrpt_eoi(u32 intrptNo) {
	/* do we have to send EOI? */
	if(intrptNo >= IRQ_MASTER_BASE && intrptNo <= IRQ_MASTER_BASE + IRQ_NUM) {
	    if(intrptNo >= IRQ_SLAVE_BASE) {
	    	/* notify the slave */
	        outb(PIC_SLAVE_CMD,PIC_EOI);
	    }

	    /* notify the master */
	    outb(PIC_MASTER_CMD,PIC_EOI);
    }
}
