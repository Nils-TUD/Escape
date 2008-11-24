/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVINTRPT_H_
#define PRIVINTRPT_H_

#include "../pub/common.h"
#include "../pub/intrpt.h"

#define IDT_COUNT		256
/* the privilege level */
#define IDT_DPL_KERNEL	0
#define IDT_DPL_USER	3
/* reserved by intel */
#define IDT_INTEL_RES1	2
#define IDT_INTEL_RES2	15
/* the code-selector */
#define IDT_CODE_SEL	0x8

/* I/O ports for PICs */
#define PIC_MASTER		0x20				/* base-port for master PIC */
#define PIC_SLAVE		0xA0				/* base-port for slave PIC */
#define PIC_MASTER_CMD	PIC_MASTER			/* command-port for master PIC */
#define PIC_MASTER_DATA	(PIC_MASTER + 1)	/* data-port for master PIC */
#define PIC_SLAVE_CMD	PIC_SLAVE			/* command-port for slave PIC */
#define PIC_SLAVE_DATA	(PIC_SLAVE + 1)		/* data-port for slave PIC */

#define PIC_EOI			0x20				/* end of interrupt */

/* flags in Initialization Command Word 1 (ICW1) */
#define ICW1_NEED_ICW4	0x01				/* ICW4 needed */
#define ICW1_SINGLE		0x02				/* Single (not cascade) mode */
#define ICW1_INTERVAL4	0x04				/* Call address interval 4 (instead of 8) */
#define ICW1_LEVEL		0x08				/* Level triggered (not edge) mode */
#define ICW1_INIT		0x10				/* Initialization - required! */

/* flags in Initialization Command Word 4 (ICW4) */
#define ICW4_8086		0x01				/* 8086/88 (instead of MCS-80/85) mode */
#define ICW4_AUTO		0x02				/* Auto (instead of normal) EOI */
#define ICW4_BUF_SLAVE	0x08				/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C				/* Buffered mode/master */
#define ICW4_SFNM		0x10				/* Special fully nested */

/* maximum number of a exception in a row */
#define MAX_EX_COUNT	3

/* represents an IDT-entry */
typedef struct {
	/* The address[0..15] of the ISR */
	u16 offsetLow;
	/* Code selector that the ISR will use */
	u16 selector;
	/* these bits are fix: 0.1110.0000.0000b */
	u16 fix		: 13,
	/* the privilege level, 00 = ring0, 01 = ring1, 10 = ring2, 11 = ring3 */
	dpl			: 2,
	/* If Present is not set to 1, an exception will occur */
	present		: 1;
	/* The address[16..31] of the ISR */
	u16	offsetHigh;
} __attribute__((packed)) sIDTEntry;

/* represents an IDT-pointer */
typedef struct {
	u16 size;
	u32 address;
} __attribute__((packed)) sIDTPtr;

/**
 * Assembler routine to load an IDT
 */
extern void intrpt_loadidt(sIDTPtr *idt);

/**
 * Our ISRs
 */
typedef void (*fISR)(void);
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);
extern void isr32(void);
extern void isr33(void);
extern void isr34(void);
extern void isr35(void);
extern void isr36(void);
extern void isr37(void);
extern void isr38(void);
extern void isr39(void);
extern void isr40(void);
extern void isr41(void);
extern void isr42(void);
extern void isr43(void);
extern void isr44(void);
extern void isr45(void);
extern void isr46(void);
extern void isr47(void);
extern void isr48(void);
extern void isr49(void);
extern void isr50(void);
extern void isr51(void);
extern void isr52(void);
extern void isr53(void);
extern void isr54(void);
extern void isr55(void);
extern void isr56(void);
extern void isr57(void);
extern void isr58(void);
extern void isr59(void);
extern void isr60(void);
extern void isr61(void);
extern void isr62(void);
extern void isr63(void);
extern void isr64(void);
extern void isr65(void);
extern void isr66(void);
extern void isr67(void);
extern void isr68(void);
extern void isr69(void);
extern void isr70(void);
extern void isr71(void);
extern void isr72(void);
extern void isr73(void);
extern void isr74(void);
extern void isr75(void);
extern void isr76(void);
extern void isr77(void);
extern void isr78(void);
extern void isr79(void);
extern void isr80(void);
extern void isr81(void);
extern void isr82(void);
extern void isr83(void);
extern void isr84(void);
extern void isr85(void);
extern void isr86(void);
extern void isr87(void);
extern void isr88(void);
extern void isr89(void);
extern void isr90(void);
extern void isr91(void);
extern void isr92(void);
extern void isr93(void);
extern void isr94(void);
extern void isr95(void);
extern void isr96(void);
extern void isr97(void);
extern void isr98(void);
extern void isr99(void);
extern void isr100(void);
extern void isr101(void);
extern void isr102(void);
extern void isr103(void);
extern void isr104(void);
extern void isr105(void);
extern void isr106(void);
extern void isr107(void);
extern void isr108(void);
extern void isr109(void);
extern void isr110(void);
extern void isr111(void);
extern void isr112(void);
extern void isr113(void);
extern void isr114(void);
extern void isr115(void);
extern void isr116(void);
extern void isr117(void);
extern void isr118(void);
extern void isr119(void);
extern void isr120(void);
extern void isr121(void);
extern void isr122(void);
extern void isr123(void);
extern void isr124(void);
extern void isr125(void);
extern void isr126(void);
extern void isr127(void);
extern void isr128(void);
extern void isr129(void);
extern void isr130(void);
extern void isr131(void);
extern void isr132(void);
extern void isr133(void);
extern void isr134(void);
extern void isr135(void);
extern void isr136(void);
extern void isr137(void);
extern void isr138(void);
extern void isr139(void);
extern void isr140(void);
extern void isr141(void);
extern void isr142(void);
extern void isr143(void);
extern void isr144(void);
extern void isr145(void);
extern void isr146(void);
extern void isr147(void);
extern void isr148(void);
extern void isr149(void);
extern void isr150(void);
extern void isr151(void);
extern void isr152(void);
extern void isr153(void);
extern void isr154(void);
extern void isr155(void);
extern void isr156(void);
extern void isr157(void);
extern void isr158(void);
extern void isr159(void);
extern void isr160(void);
extern void isr161(void);
extern void isr162(void);
extern void isr163(void);
extern void isr164(void);
extern void isr165(void);
extern void isr166(void);
extern void isr167(void);
extern void isr168(void);
extern void isr169(void);
extern void isr170(void);
extern void isr171(void);
extern void isr172(void);
extern void isr173(void);
extern void isr174(void);
extern void isr175(void);
extern void isr176(void);
extern void isr177(void);
extern void isr178(void);
extern void isr179(void);
extern void isr180(void);
extern void isr181(void);
extern void isr182(void);
extern void isr183(void);
extern void isr184(void);
extern void isr185(void);
extern void isr186(void);
extern void isr187(void);
extern void isr188(void);
extern void isr189(void);
extern void isr190(void);
extern void isr191(void);
extern void isr192(void);
extern void isr193(void);
extern void isr194(void);
extern void isr195(void);
extern void isr196(void);
extern void isr197(void);
extern void isr198(void);
extern void isr199(void);
extern void isr200(void);
extern void isr201(void);
extern void isr202(void);
extern void isr203(void);
extern void isr204(void);
extern void isr205(void);
extern void isr206(void);
extern void isr207(void);
extern void isr208(void);
extern void isr209(void);
extern void isr210(void);
extern void isr211(void);
extern void isr212(void);
extern void isr213(void);
extern void isr214(void);
extern void isr215(void);
extern void isr216(void);
extern void isr217(void);
extern void isr218(void);
extern void isr219(void);
extern void isr220(void);
extern void isr221(void);
extern void isr222(void);
extern void isr223(void);
extern void isr224(void);
extern void isr225(void);
extern void isr226(void);
extern void isr227(void);
extern void isr228(void);
extern void isr229(void);
extern void isr230(void);
extern void isr231(void);
extern void isr232(void);
extern void isr233(void);
extern void isr234(void);
extern void isr235(void);
extern void isr236(void);
extern void isr237(void);
extern void isr238(void);
extern void isr239(void);
extern void isr240(void);
extern void isr241(void);
extern void isr242(void);
extern void isr243(void);
extern void isr244(void);
extern void isr245(void);
extern void isr246(void);
extern void isr247(void);
extern void isr248(void);
extern void isr249(void);
extern void isr250(void);
extern void isr251(void);
extern void isr252(void);
extern void isr253(void);
extern void isr254(void);
extern void isr255(void);

#endif /* PRIVINTRPT_H_ */
