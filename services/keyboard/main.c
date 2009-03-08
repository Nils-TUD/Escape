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

/* the interrupt-msg */
static sMsgKbRequest kbIntrptMsg = {
	.id = KEYBOARD_MSG_INTRPT
};

s32 main(void) {
	s32 selfFd;
	s32 id = regService("keyboard",SERVICE_TYPE_SINGLEPIPE);
	if(id < 0) {
		printLastError();
		return 1;
	}

	if(addIntrptListener(id,IRQ_KEYBOARD,&kbIntrptMsg,sizeof(sMsgKbRequest)) < 0)
		printLastError();
	else
		debugf("Announced keyboard interrupt-handler\n");

	if(requestIOPort(IOPORT_PIC) < 0)
		printLastError();
	else
		debugf("Reserved IO-port 0x%02x\n",IOPORT_PIC);
	if(requestIOPort(IOPORT_KB_CTRL) < 0)
		printLastError();
	else
		debugf("Reserved IO-port 0x%02x\n",IOPORT_KB_CTRL);

	selfFd = open("services:/keyboard",IO_WRITE);
	if(selfFd < 0) {
		printLastError();
		return 1;
	}

	static sMsgKbResponse resp;
	static sMsgKbRequest msg;
	while(1) {
		s32 fd = waitForClient(id);
		if(fd < 0)
			printLastError();
		else {
			s32 c = 0;
			do {
				if((c = read(fd,&msg,sizeof(sMsgKbRequest))) < 0)
					printLastError();
				else if(c > 0) {
					if(msg.id == KEYBOARD_MSG_INTRPT) {
						u8 scanCode = inb(IOPORT_KB_CTRL);
						if(kb_set1_getKeycode(&resp,scanCode)) {
							write(selfFd,&resp,sizeof(sMsgKbResponse));
							/*debugf("Got scancode %d => keycode %d\n",scanCode,(buffer + writePos)->keycode);*/
						}
						/* ack scancode */
						outb(IOPORT_PIC,PIC_ICW1);
					}
				}
			}
			while(c > 0);
			close(fd);
		}
	}

	releaseIOPort(IOPORT_PIC);
	releaseIOPort(IOPORT_KB_CTRL);

	unregService(id);
	return 0;
}
