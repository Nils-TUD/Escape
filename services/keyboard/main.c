/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <messages.h>
#include <service.h>
#include <io.h>
#include <heap.h>
#include <string.h>
#include <debug.h>
#include <proc.h>

#include "set1.h"

/* io-ports */
#define IOPORT_PIC			0x20
#define IOPORT_KB_CTRL		0x60

/* ICW = initialisation command word */
#define PIC_ICW1			0x20

/* the size of the keycode-buffer */
#define BUFFER_SIZE			256
/* the keyboard msg-id */
#define KEYBOARD_MSG_INTRPT	0xFF

/* a request-message for the keyboard-service */
typedef struct {
	/* the message-id */
	u8 id;
} sMsgKbRequest;

/* the interrupt-msg */
static sMsgKbRequest kbIntrptMsg = {
	.id = KEYBOARD_MSG_INTRPT
};

s32 main(void) {
	s32 selfFd;
	s32 id;

	debugf("Service keyboard has pid %d\n",getpid());

	id = regService("keyboard",SERVICE_TYPE_SINGLEPIPE);
	if(id < 0) {
		printLastError();
		return 1;
	}

	/* we want to get notified about keyboard interrupts */
	if(addIntrptListener(id,IRQ_KEYBOARD,&kbIntrptMsg,sizeof(sMsgKbRequest)) < 0) {
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

	static sMsgKbResponse resp;
	static sMsgKbRequest msg;
	while(1) {
		s32 fd = getClient(id);
		if(fd < 0) {
			sleep();
		}
		else {
			s32 c = 0;
			do {
				if((c = read(fd,&msg,sizeof(sMsgKbRequest))) < 0)
					printLastError();
				else if(c > 0) {
					if(msg.id == KEYBOARD_MSG_INTRPT) {
						u8 scanCode = inByte(IOPORT_KB_CTRL);
						if(kb_set1_getKeycode(&resp,scanCode)) {
							/* write in receive-pipe */
							write(selfFd,&resp,sizeof(sMsgKbResponse));
						}
						/* ack scancode */
						outByte(IOPORT_PIC,PIC_ICW1);
					}
				}
			}
			while(c > 0);
			close(fd);
		}
	}

	/* clean up */
	releaseIOPort(IOPORT_PIC);
	releaseIOPort(IOPORT_KB_CTRL);

	unregService(id);
	return 0;
}
