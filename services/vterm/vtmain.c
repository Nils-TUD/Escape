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

	static sMsgKbResponse keycode;
	static sMsgDefHeader msg;
	while(1) {
		s32 fd = getClient(id);
		if(fd < 0) {
			/* read from keyboard */
			while(read(kbFd,&keycode,sizeof(sMsgKbResponse)) > 0) {
				vterm_handleKeycode(&keycode);
			}
			sleep(EV_CLIENT | EV_RECEIVED_MSG);
		}
		else {
			while(read(fd,&msg,sizeof(sMsgDefHeader)) > 0) {
				switch(msg.id) {
					case MSG_VTERM_WRITE: {
						if(msg.length > 0) {
							s8 *buffer = malloc(msg.length * sizeof(s8));
							if(read(fd,buffer,msg.length) < 0)
								printLastError();
							else {
								*(buffer + msg.length - 1) = '\0';
								vterm_puts(buffer);
							}
							free(buffer);
						}
					}
					break;
				}
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
