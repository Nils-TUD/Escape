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

s32 main(void) {
	s32 id = regService("console");
	if(id < 0) {
		printLastError();
		return 1;
	}

	vid_init();

	u32 msgCount = 0;
	static sMsgConRequest msg;
	while(1) {
		s32 fd = waitForClient(id);
		if(fd < 0)
			printLastError();
		else {
			s32 x = 0,c = 0;
			do {
				if((c = read(fd,&msg,sizeof(sMsgConRequest))) < 0)
					printLastError();
				else if(c > 0) {
					if(msg.id == MSG_CONSOLE_OUT) {
						s8 *readBuf = malloc(msg.length * sizeof(s8));
						read(fd,readBuf,msg.length);
						free(readBuf);
						vid_printf("Read (%d, %d bytes): %s\n",x,msg.length,readBuf);
						/*msgCount++;
						if(msgCount % 1000 == 0)
							vid_printf("Got %d messages\n",msgCount);*/
					}
					else if(msg.id == MSG_CONSOLE_IN) {

					}
					else if(msg.id == MSG_CONSOLE_CLEAR) {
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
