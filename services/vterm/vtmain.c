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
#include <sllist.h>

#include "vterm.h"

/* our read-buffer */
#define BUFFER_SIZE 64
static s8 buffer[BUFFER_SIZE + 1];

s32 main(void) {
	s32 kbFd;
	s32 id;

	id = regService("vterm",SERVICE_TYPE_SINGLEPIPE);
	if(id < 0) {
		printLastError();
		return 1;
	}

	/* open keyboard */
	kbFd = open("services:/keyboard",IO_READ);
	if(kbFd < 0) {
		printLastError();
		return 1;
	}

	/* request io-ports for qemu and bochs */
	requestIOPort(0xe9);
	requestIOPort(0x3f8);
	requestIOPort(0x3fd);

	vterm_init();

	sMsgKbResponse keycode;
	while(1) {
		s32 fd = getClient(id);
		if(fd < 0) {
			/* read from keyboard */
			while(!eof(kbFd)) {
				read(kbFd,&keycode,sizeof(sMsgKbResponse));
				vterm_handleKeycode(&keycode);
			}
			sleep(EV_CLIENT | EV_RECEIVED_MSG);
		}
		else {
			u32 c;
			while((c = read(fd,buffer,BUFFER_SIZE)) > 0) {
				*(buffer + c) = '\0';
				vterm_puts(buffer);
			}
			close(fd);
		}
	}

	/* clean up */
	releaseIOPort(0xe9);
	releaseIOPort(0x3f8);
	releaseIOPort(0x3fd);
	vterm_destroy();
	close(kbFd);
	unregService(id);
	return 0;
}
