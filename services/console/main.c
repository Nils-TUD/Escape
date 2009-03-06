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

#include <video.h>

/* the keyboard msg-id */
#define CONSOLE_MSG_KB_INTRPT 0xFF

/* the keyboard-interrupt-msg */
static sConsoleMsg kbIntrptMsg = {
	.id = CONSOLE_MSG_KB_INTRPT,
	.length = 0
};

s32 main(void) {
	s32 id = regService("console");
	if(id < 0) {
		printLastError();
		return 1;
	}

	vid_init();

	if(addIntrptListener(id,IRQ_KEYBOARD,&kbIntrptMsg,sizeof(sConsoleMsg)) < 0)
		printLastError();
	else
		vid_printf("Announced keyboard interrupt-handler\n");

	if(requestIOPort(0x20) < 0)
		printLastError();
	else
		vid_printf("Reserved IO-port 0x20\n");
	if(requestIOPort(0x60) < 0)
		printLastError();
	else
		vid_printf("Reserved IO-port 0x60\n");

	u32 msgCount = 0;
	static sConsoleMsg msg;
	while(1) {
		s32 fd = waitForClient(id);
		if(fd < 0)
			printLastError();
		else {
			u32 x = 0;
			s32 c = 0;
			do {
				if((c = read(fd,&msg,sizeof(sConsoleMsg))) < 0)
					printLastError();
				else if(c > 0) {
					if(msg.id == CONSOLE_MSG_KB_INTRPT) {
						u8 scanCode = inb(0x60);
						kb_handleScanCode(scanCode);
						/*vid_printf("\e[31mKEYBOARD INTERRUPT (scancode %d)!!\e[0m\n",scanCode);*/
						/* ack scancode */
						outb(0x20,0x20);
					}
					else if(msg.id == CONSOLE_MSG_OUT) {
						s8 *readBuf = malloc(msg.length * sizeof(s8));
						read(fd,readBuf,msg.length);
						free(readBuf);
						/*vid_printf("Read (%d, %d bytes): %s\n",x,msg.length,readBuf);*/
						msgCount++;
						if(msgCount % 1000 == 0)
							vid_printf("Got %d messages\n",msgCount);
						/*s8 *readBuf = malloc(msg.length * sizeof(s8));
						read(fd,readBuf,msg.length);
						vid_printf("Read (%d, %d bytes): %s\n",x,msg.length,readBuf);
						free(readBuf);*/
					}
					else if(msg.id == CONSOLE_MSG_IN) {

					}
					else if(msg.id == CONSOLE_MSG_CLEAR) {
						vid_clearScreen();
					}
					x++;
				}
			}
			while(c > 0);
			close(fd);
		}
	}

	releaseIOPort(0x20);
	releaseIOPort(0x60);

	unregService(id);
	return 0;
}
