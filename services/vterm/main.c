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
	sSLList *rlWaits;

	debugf("Service vterm has pid %d\n",getpid());

	id = regService("vterm",SERVICE_TYPE_MULTIPIPE);
	if(id < 0) {
		printLastError();
		return 1;
	}

	/* open keyboard */
	do {
		kbFd = open("services:/keyboard",IO_READ);
		if(kbFd < 0)
			yield();
	}
	while(kbFd < 0);

	/* request io-ports for qemu and bochs */
	requestIOPort(0xe9);
	requestIOPort(0x3f8);
	requestIOPort(0x3fd);

	rlWaits = sll_create();
	if(rlWaits == NULL) {
		printLastError();
		return 1;
	}

	vterm_init();

	static sMsgKbResponse keycode;
	static sMsgDefHeader msg;
	static sMsgDataVTermReadLine rlData;
	while(1) {
		s32 fd = getClient(id);
		if(fd < 0) {
			u16 length;
			s8 *line;
			/* read from keyboard */
			while(read(kbFd,&keycode,sizeof(sMsgKbResponse)) > 0) {
				vterm_handleKeycode(&keycode);
				/* TODO improve that! */
				if(sll_length(rlWaits) > 0 && (length = vterm_getReadLength()) > 0) {
					sMsgDefHeader *reply = createDefMsg(MSG_VTERM_READLINE_REPL,length,vterm_getReadLine());
					tFD waitingFd = (tFD)sll_get(rlWaits,0);
					write(waitingFd,reply,sizeof(sMsgDefHeader) + reply->length);
					freeDefMsg(reply);
					sll_removeIndex(rlWaits,0);
					close(waitingFd);
				}
			}
			sleep();
		}
		else {
			bool closeFD = true;
			s32 c = 0;
			do {
				if((c = read(fd,&msg,sizeof(sMsgDefHeader))) < 0)
					printLastError();
				else if(c > 0) {
					if(msg.id == MSG_VTERM_WRITE) {
						if(msg.length > 0) {
							s8 *buffer = malloc(msg.length * sizeof(s8));
							read(fd,buffer,msg.length);
							*(buffer + msg.length - 1) = '\0';
							vterm_puts(buffer);
							free(buffer);
						}
					}
					else if(msg.id == MSG_VTERM_READLINE) {
						read(fd,&rlData,sizeof(sMsgDataVTermReadLine));
						/* TODO should we really ignore duplicate reads? */
						if(!vterm_isReading()) {
							vterm_startReading(rlData.maxLength);
							/* put client in the waiting queue */
							sll_append(rlWaits,fd);
							closeFD = false;
						}
					}
				}
			}
			while(c > 0);

			if(closeFD)
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
