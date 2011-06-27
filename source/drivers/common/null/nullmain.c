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
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <stdio.h>
#include <errors.h>
#include <stdlib.h>

static sMsg msg;

int main(void) {
	int id;
	msgid_t mid;

	id = regDriver("null",DRV_READ | DRV_WRITE);
	if(id < 0)
		error("Unable to register driver 'null'");

	/* /dev/null produces no output, so always available to prevent blocking */
	if(fcntl(id,F_SETDATA,true) < 0)
		error("fcntl");

    /* wait for commands */
	while(1) {
		int fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[NULL] Unable to get work");
		else {
			switch(mid) {
				case MSG_DRV_READ:
					msg.args.arg1 = 0;
					msg.args.arg2 = true;
					send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
					break;
				case MSG_DRV_WRITE:
					/* skip the data-message */
					if(RETRY(receive(fd,NULL,NULL,0)) < 0)
						printe("[NULL] Unable to skip data-msg");
					/* write response and pretend that we've written everything */
					msg.args.arg1 = msg.args.arg2;
					send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
					break;
				default:
					msg.args.arg1 = ERR_UNSUPPORTED_OP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}

	/* clean up */
	close(id);
	return EXIT_SUCCESS;
}
