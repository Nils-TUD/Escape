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

/* the keycode-buffer */
static u32 keyCount = 0;
static u32 readPos = 0;
static u32 writePos = 0;
static sMsgKbResponse buffer[BUFFER_SIZE];

s32 main(void) {
	s32 id = regService("keyboard");
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
						/* overwrite old scancodes if the buffer is full */
						if(kb_set1_getKeycode(buffer + writePos,scanCode)) {
							/*debugf("Got scancode %d => keycode %d\n",scanCode,(buffer + writePos)->keycode);*/
							writePos = (writePos + 1) % BUFFER_SIZE;
							if(keyCount == BUFFER_SIZE)
								readPos = (readPos + 1) % BUFFER_SIZE;
							else
								keyCount++;
						}
						/* ack scancode */
						outb(IOPORT_PIC,PIC_ICW1);
					}
					else if(msg.id == KEYBOARD_MSG_READ) {
						/* the client has to wait until there is a keycode */
						if(keyCount > 0) {
							if(write(fd,buffer + readPos,sizeof(sMsgKbResponse)) > 0) {
								readPos = (readPos + 1) % BUFFER_SIZE;
								keyCount--;
							}
						}
					}
				}
			}
			while(c > 0);
			close(fd);

			/*debugf("writePos=%d, readPos=%d, keyCount=%d\n",writePos,readPos,keyCount);*/
		}
	}

	releaseIOPort(IOPORT_PIC);
	releaseIOPort(IOPORT_KB_CTRL);

	unregService(id);
	return 0;
}
