/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/intrpt.h"
#include "../h/util.h"
#include "../h/keyboard.h"
#include "../h/cpu.h"
#include "../h/paging.h"
#include "../h/proc.h"
#include "../h/elf.h"
#include "../h/syscalls.h"
#include "../h/vfs.h"
#include "../h/gdt.h"
#include "../h/kheap.h"
#include <string.h>
#include <sllist.h>
#include <video.h>

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

/* the maximum length of messages (for interrupt-listeners) */
#define MSG_MAX_LEN		8

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

/* the stuff we need to know about interrupt-listeners */
typedef struct {
	sVFSNode *node;
	tFile file;
	u8 pending;
	u8 msgLen;
	u8 message[MSG_MAX_LEN];
} sIntrptListener;

/* isr prototype */
typedef void (*fISR)(void);

/**
 * Assembler routine to load an IDT
 */
extern void intrpt_loadidt(sIDTPtr *idt);

/**
 * Our ISRs
 */
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
/* the handler for a other interrupts */
extern void isrNull(void);

/**
 * Inits the programmable interrupt controller
 */
static void intrpt_initPic(void);

/**
 * Sets the IDT-entry for the given interrupt
 *
 * @param number the interrupt-number
 * @param handler the ISR
 * @param dpl the privilege-level
 */
static void intrpt_setIDT(u16 number,fISR handler,u8 dpl);

/**
 * Sends EOI to the PIC, if necessary
 *
 * @param intrptNo the interrupt-number
 */
static void intrpt_eoi(u32 intrptNo);

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
	/* 0x2F */	"ATA2",
	/* 0x30 */	"Syscall"
};

/* stuff to count exceptions */
static u32 exCount = 0;
static u32 lastEx = 0xFFFFFFFF;

/**
 * A linked list for each hardware-interrupt with vfs-nodes that should be "notified" as soon
 * as an interrupt occurres. The linked list will be NULL if it was not used yet.
 */
static sSLList *intrptListener[IRQ_NUM];

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
	u32 i;
	/* setup the idt-pointer */
	sIDTPtr idtPtr;
	idtPtr.address = (u32)idt;
	idtPtr.size = sizeof(idt) - 1;

	/* setup the idt */

	/* exceptions */
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

	/* hardware-interrupts */
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

	/* syscall */
	intrpt_setIDT(48,isr48,IDT_DPL_USER);

	/* all other interrupts */
	for(i = 49; i < 256; i++)
		intrpt_setIDT(i,isrNull,IDT_DPL_KERNEL);

	/* now we can use our idt */
	intrpt_loadidt(&idtPtr);

	/* now init the PIC */
	intrpt_initPic();
}

s32 intrpt_addListener(u16 irq,void *node,void *message,u32 msgLen) {
	sIntrptListener *l;
	sSLList *list;
	s32 err;
	if(msgLen > MSG_MAX_LEN)
		return ERR_INTRPT_LISTENER_MSGLEN;

	/* check irq */
	if(irq < IRQ_MASTER_BASE || irq >= IRQ_MASTER_BASE + IRQ_NUM)
		return ERR_INVALID_IRQ_NUMBER;

	/* TODO: check wether the node has already announced a listener for this IRQ */

	/* reserve space for the list-element */
	l = kheap_alloc(sizeof(sIntrptListener));
	if(l == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* init list-element */
	memcpy(l->message,message,msgLen);
	l->msgLen = msgLen;
	l->node = node;
	/* create a node for it and open it */
	err = vfs_openIntrptMsgNode(node);
	if(err < 0) {
		kheap_free(l);
		return err;
	}
	l->file = err;
	l->pending = 0;

	list = intrptListener[irq - IRQ_MASTER_BASE];
	/* no list present yet? */
	if(list == NULL) {
		list = sll_create();
		if(list == NULL) {
			/* close file and free element */
			vfs_closeIntrptMsgNode(l->file);
			kheap_free(l);
			return ERR_NOT_ENOUGH_MEM;
		}
		/* store */
		intrptListener[irq - IRQ_MASTER_BASE] = list;
	}

	sll_append(list,l);
	return 0;
}

s32 intrpt_removeListener(u16 irq,void *node) {
	sSLList *list;
	sSLNode *ln,*lnp;
	/* check irq */
	if(irq < IRQ_MASTER_BASE || irq >= IRQ_MASTER_BASE + IRQ_NUM)
		return ERR_INVALID_IRQ_NUMBER;

	/* get list */
	list = intrptListener[irq - IRQ_MASTER_BASE];
	if(list == NULL)
		return ERR_IRQ_LISTENER_MISSING;

	/* search for the node */
	lnp = NULL;
	for(ln = sll_begin(list); ln != NULL; ln = ln->next) {
		if(((sIntrptListener*)ln->data)->node == node) {
			sll_removeNode(list,ln,lnp);
			vfs_closeIntrptMsgNode(((sIntrptListener*)ln->data)->file);
			/* free the data */
			kheap_free(ln->data);
			return 0;
		}
		lnp = ln;
	}

	return ERR_IRQ_LISTENER_MISSING;
}


/* TODO temporary */
typedef struct {
	s8 name[MAX_PROC_NAME_LEN + 1];
	u8 *data;
} sProcData;
#include "../../build/services.txt"
static bool servicesLoaded = false;
static bool firstProcLoaded = false;

/*static u8 task2[] = {
	#include "../../build/user_task2.dump"
};
static bool proc2Ready = false;*/

static u64 umodeTime = 0;
static u64 kmodeTime = 0;
static u64 kmodeStart = 0;
static u64 kmodeEnd = 0;

static bool intrptsPending = false;

static void intrpt_handleListener(u16 irq,sSLList *list) {
	sSLNode *node;
	sIntrptListener *l;
	bool oldState;
	u32 i;
	/* first increase the number of pending interrupts (there may be some already) */
	for(node = sll_begin(list); node != NULL; node = node->next) {
		l = (sIntrptListener*)node->data;
		l->pending++;
	}
	/* now handle them */
	for(node = sll_begin(list); node != NULL; node = node->next) {
		l = (sIntrptListener*)node->data;
		while(l->pending > 0) {
			/* send message (use PROC_COUNT to write to the service) */
			vfs_writeFile(PROC_COUNT,vfs_getFile(l->file),l->message,l->msgLen);
			l->pending--;
		}
	}
}

void intrpt_handler(sIntrptStackFrame stack) {
	sProc *p;
	/*u32 syscallNo = stack.intrptNo == IRQ_SYSCALL ? ((sSysCallStack*)stack.uesp)->number : 0;*/

	kmodeStart = cpu_rdtsc();
	umodeTime += kmodeStart - kmodeEnd;

	/* check if we have listeners for this interrupt */
	if(stack.intrptNo >= IRQ_MASTER_BASE && stack.intrptNo < IRQ_MASTER_BASE + IRQ_NUM) {
		sSLList *list = intrptListener[stack.intrptNo - IRQ_MASTER_BASE];
		if(list != NULL)
			intrpt_handleListener(stack.intrptNo,list);
	}
	/* are there undelivered interrupts? */
	else if(intrptsPending) {
		intrptsPending = false;
		sSLList *list;
		u32 i;
		/* walk through all lists to find pending interrupts */
		/* TODO should we speed that up? */
		for(i = 0; i < IRQ_NUM; i++) {
			list = intrptListener[i];
			if(list != NULL)
				intrpt_handleListener(i + IRQ_MASTER_BASE,list);
		}
	}

	/*vid_printf("umodeTime=%d%%\n",(s32)(100. / (cpu_rdtsc() / (double)umodeTime)));*/
	switch(stack.intrptNo) {
		case IRQ_KEYBOARD:
			/*kbd_handleIntrpt();*/
			break;

		case IRQ_TIMER:
			/* TODO don't resched if we come from kernel-mode! */
			ASSERT(stack.ds == 0x23,"Timer interrupt from kernel-mode!");

			if(!servicesLoaded || !firstProcLoaded) {
				u32 i = 0xFFFFFFFF,end;

				/* TODO a little hack that loads the services and processes as soon as
				 * the first process (init) has set stderr */
				/* later init will load the services and so on. therefore this will no longer be
				 * necessary */
				if(!firstProcLoaded && proc_getRunning()->pid == 0 && proc_getByPid(0)->fileDescs[2] != -1) {
					/* load first process */
					i = 0;
					end = 1;
					firstProcLoaded = true;
					/*vid_printf("Loading first proc...\n");*/
				}
				else if(!servicesLoaded) {
					/* load services */
					i = 1;
					end = ARRAY_SIZE(services);
					servicesLoaded = true;
					/*vid_printf("Loading services...\n");*/
				}

				if(i != 0xFFFFFFFF) {
					for(; i < end; i++) {
						/* clone proc */
						tPid pid = proc_getFreePid();
						if(proc_clone(pid)) {
							/* we'll reach this as soon as the scheduler has chosen the created process */
							p = proc_getRunning();
							/* remove data-pages */
							proc_changeSize(-p->dataPages,CHG_DATA);
							/* now load service */
							/*vid_printf("Loading service %d\n",p->pid);*/
							/* TODO just temporary */
							memcpy(p->name,services[i].name,strlen(services[i].name) + 1);
							elf_loadprog(services[i].data);
							/*vid_printf("Starting...\n");*/
							proc_setupIntrptStack(&stack);
							/* we don't want to continue the loop ;) */
							break;
						}
					}
					/*vid_printf("Done\n");*/
				}
				break;
			}

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
				/*vid_printf("Page fault for address=0x%08x @ 0x%x, process %d\n",cpu_getCR2(),
						stack.eip,proc_getRunning()->pid);*/
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
				p = proc_getRunning();
				/* io-map not loaded yet? */
				if(p->ioMap != NULL && !tss_ioMapPresent()) {
					/* load it and give the process another try */
					tss_setIOMap(p->ioMap);
					exCount = 0;
					break;
				}

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

	kmodeEnd = cpu_rdtsc();
	kmodeTime += kmodeEnd - kmodeStart;
	/*if((u32)(kmodeEnd - kmodeStart) > 2000000)
		vid_printf("SLOW(%d) %d %d\n",(u32)(kmodeEnd - kmodeStart),stack.intrptNo,syscallNo);
	else
		vid_printf("FAST(%d) %d %d\n",(u32)(kmodeEnd - kmodeStart),stack.intrptNo,syscallNo);*/
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
