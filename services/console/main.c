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

#include "cvideo.h"

s32 main(void) {
	s32 id = regService("console");
	if(id < 0) {
		printLastError();
		return 1;
	}

	vid_init();

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
					if(msg.id == CONSOLE_MSG_OUT) {
						s8 *readBuf = malloc(msg.length * sizeof(s8));
						read(fd,readBuf,msg.length);
						vid_printf("Read (%d): ",x);
						vid_printf(readBuf);
						vid_printf("\n");
						free(readBuf);
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

	unregService(id);
	return 0;
}
