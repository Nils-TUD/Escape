/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
#include <stdlib.h>
#include <sllist.h>
#include <string.h>

#include "vterm.h"

/* read buffer size */
#define READ_BUF_SIZE 256

/**
 * Determines the vterm for the given service-id
 *
 * @param sid the service-id
 * @return the vterm or NULL
 */
static sVTerm *getVTerm(tServ sid);

/* wether we should read from keyboard */
static bool readKeyboard = true;

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
			printe("Unable to register service '%s'",name);
			return EXIT_FAILURE;
		}
	}

	/* init vterms */
	if(!vterm_initAll()) {
		fprintf(stderr,"Unable to init vterms");
		return EXIT_FAILURE;
	}

	/* open keyboard */
	kbFd = open("services:/keyboard",IO_READ);
	if(kbFd < 0) {
		printe("Unable to open 'services:/keyboard'");
		return EXIT_FAILURE;
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
			if(readKeyboard) {
				/* read from keyboard */
				/* don't block here since there may be waiting clients.. */
				while(!eof(kbFd)) {
					read(kbFd,&keycode,sizeof(sMsgKbResponse));
					vterm_handleKeycode(&keycode);
				}
				wait(EV_CLIENT | EV_RECEIVED_MSG);
			}
			else
				wait(EV_CLIENT);
		}
		else {
			sVTerm *vt = getVTerm(client);
			if(vt != NULL) {
				u32 c;
				/* TODO this may cause trouble with escape-codes. maybe we should store the
				 * "escape-state" somehow... */
				while((c = read(fd,buffer,READ_BUF_SIZE)) > 0) {
					*(buffer + c) = '\0';
					vterm_puts(vt,buffer,c,true,&readKeyboard);
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
	return EXIT_SUCCESS;
}

static sVTerm *getVTerm(tServ sid) {
	u32 i;
	for(i = 0; i < VTERM_COUNT; i++) {
		if(servIds[i] == sid)
			return vterm_get(i);
	}
	return NULL;
}
