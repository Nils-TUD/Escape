/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <messages.h>
#include <service.h>
#include <io.h>
#include <ports.h>
#include <heap.h>
#include <string.h>
#include <debug.h>
#include <proc.h>
#include <signals.h>

#include "set1.h"

/* io-ports */
#define IOPORT_PIC			0x20
#define IOPORT_KB_CTRL		0x60

/* ICW = initialisation command word */
#define PIC_ICW1			0x20

/**
 * The keyboard-interrupt handler
 */
static void kbIntrptHandler(tSig sig,u32 data);

/* file-descriptor for ourself */
static tFD selfFd;

int main(void) {
	tServ id;

	id = regService("keyboard",SERVICE_TYPE_SINGLEPIPE);
	if(id < 0) {
		printLastError();
		return 1;
	}

	/* request io-ports */
	if(requestIOPort(IOPORT_PIC) < 0) {
		printLastError();
		return 1;
	}
	if(requestIOPort(IOPORT_KB_CTRL) < 0) {
		printLastError();
		return 1;
	}

	/* open ourself to write keycodes into the receive-pipe (which can be read by other processes) */
	selfFd = open("services:/keyboard",IO_WRITE);
	if(selfFd < 0) {
		printLastError();
		return 1;
	}

	/* we want to get notified about keyboard interrupts */
	if(setSigHandler(SIG_INTRPT_KB,kbIntrptHandler) < 0) {
		printLastError();
		return 1;
	}

    /* reset keyboard */
    outByte(IOPORT_KB_CTRL,0xff);
    while(inByte(IOPORT_KB_CTRL) != 0xaa)
    	yield();

	/* we don't want to be waked up. we'll get signals anyway */
	while(1)
		wait(EV_NOEVENT);

	/* clean up */
	unsetSigHandler(SIG_INTRPT_KB);
	close(selfFd);
	releaseIOPort(IOPORT_PIC);
	releaseIOPort(IOPORT_KB_CTRL);
	unregService(id);

	return 0;
}

static void kbIntrptHandler(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	static sMsgKbResponse resp;
	u8 scanCode = inByte(IOPORT_KB_CTRL);
	if(kb_set1_getKeycode(&resp,scanCode)) {
		/* write in receive-pipe */
		write(selfFd,&resp,sizeof(sMsgKbResponse));
	}
	/* ack scancode */
	outByte(IOPORT_PIC,PIC_ICW1);
}
