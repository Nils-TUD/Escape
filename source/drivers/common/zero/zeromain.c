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
#include <esc/io.h>
#include <stdio.h>
#include <esc/messages.h>
#include <stdlib.h>
#include <errors.h>

static sMsg msg;

int main(void) {
	tFD id;
	tMsgId mid;

	id = regDriver("zero",DRV_READ);
	if(id < 0)
		error("Unable to register driver 'zero'");

	/* 0's are always available ;) */
	if(fcntl(id,F_SETDATA,true) < 0)
		error("fcntl");

    /* wait for commands */
	while(1) {
		tFD fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[ZERO] Unable to get work");
		else {
			switch(mid) {
				case MSG_DRV_READ: {
					/* offset is ignored here */
					size_t count = msg.args.arg2;
					void *data = calloc(count,1);
					if(!data) {
						printe("[ZERO] Unable to alloc mem");
						count = 0;
					}
					msg.args.arg1 = count;
					msg.args.arg2 = true;
					send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
					if(data) {
						send(fd,MSG_DRV_READ_RESP,data,count);
						free(data);
					}
				}
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
