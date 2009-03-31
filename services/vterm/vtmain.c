/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/messages.h>
#include <esc/service.h>
#include <esc/io.h>
#include <esc/ports.h>
#include <esc/heap.h>
#include <esc/debug.h>
#include <esc/proc.h>
#include <esc/fileio.h>
#include <sllist.h>
#include <string.h>

#include "vterm.h"

/* read buffer size */
#define READ_BUF_SIZE 64

/**
 * Determines the vterm for the given service-id
 *
 * @param sid the service-id
 * @return the vterm or NULL
 */
static sVTerm *getVTerm(tServ sid);

/* vterms */
static tServ servIds[VTERM_COUNT] = {-1};
/* read-buffer */
static char buffer[READ_BUF_SIZE + 1];

int main(void) {
	u32 i;
	tFD kbFd;
	tServ client;
	char name[MAX_NAME_LEN + 1];
	sMsgKbResponse keycode;

	/* reg services */
	for(i = 0; i < VTERM_COUNT; i++) {
		sprintf(name,"vterm%d",i);
		servIds[i] = regService(name,SERVICE_TYPE_SINGLEPIPE);
		if(servIds[i] < 0) {
			printLastError();
			return 1;
		}
	}

	/* init vterms */
	if(!vterm_initAll()) {
		debugf("Unable to init vterms\n");
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

	/* select first vterm */
	vterm_selectVTerm(0);

	while(1) {
		tFD fd = getClient(servIds,VTERM_COUNT,&client);
		if(fd < 0) {
			/* read from keyboard */
			/* don't block here since there may be waiting clients.. */
			while(!eof(kbFd)) {
				read(kbFd,&keycode,sizeof(sMsgKbResponse));
				vterm_handleKeycode(&keycode);
			}
			wait(EV_CLIENT | EV_RECEIVED_MSG);
		}
		else {
			sVTerm *vt = getVTerm(client);
			if(vt != NULL) {
				u32 c;
				while((c = read(fd,buffer,READ_BUF_SIZE)) > 0) {
					*(buffer + c) = '\0';
					vterm_puts(vt,buffer,true);
				}
			}
			close(fd);
		}
	}

	/* clean up */
	releaseIOPort(0xe9);
	releaseIOPort(0x3f8);
	releaseIOPort(0x3fd);
	close(kbFd);
	for(i = 0; i < VTERM_COUNT; i++) {
		unregService(servIds[i]);
		vterm_destroy(vterm_get(i));
	}
	return 0;
}

static sVTerm *getVTerm(tServ sid) {
	u32 i;
	for(i = 0; i < VTERM_COUNT; i++) {
		if(servIds[i] == sid)
			return vterm_get(i);
	}
	return NULL;
}
