/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/driver.h>
#include <esc/messages.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include "window.h"
#include "infodev.h"

static void getWindows(FILE *str);

int infodev_thread(A_UNUSED void *arg) {
	sMsg msg;
	msgid_t mid;
	int id;

	id = createdev("/system/windows",DEV_TYPE_FILE,DEV_READ);
	if(id < 0)
		error("Unable to create file /system/windows");
	if(chmod("/system/windows",0644) < 0)
		error("Unable to chmod /system/windows");

	while(1) {
		int fd = getwork(id,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != -EINTR)
				printe("[WININFO] Unable to get work");
		}
		else {
			switch(mid) {
				case MSG_DEV_READ:
					handleFileRead(fd,&msg,getWindows);
					break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}
	/* never reached */
	return 0;
}

static void getWindows(FILE *str) {
	size_t i;
	for(i = 0; i < WINDOW_COUNT; i++) {
		sWindow *w = win_get(i);
		if(w) {
			fprintf(str,"Window %d:\n",w->id);
			fprintf(str,"\tOwner: %d\n",w->owner);
			fprintf(str,"\tPosition: %d,%d,%d\n",w->x,w->y,w->z);
			fprintf(str,"\tSize: %u x %u\n",w->width,w->height);
			fprintf(str,"\tStyle: %#x\n",w->style);
		}
	}
}
