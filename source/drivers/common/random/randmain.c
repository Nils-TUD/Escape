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
#include <esc/messages.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errors.h>

static sMsg msg;

int main(void) {
	tFD id;
	tMsgId mid;

	id = regDriver("random",DRV_READ);
	if(id < 0)
		error("Unable to register driver 'random'");

	/* random numbers are always available ;) */
	if(fcntl(id,F_SETDATA,true) < 0)
		error("fcntl");
	srand(time(NULL));

    /* wait for commands */
	while(1) {
		tFD fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[RAND] Unable to get work");
		else {
			switch(mid) {
				case MSG_DRV_READ: {
					/* offset is ignored here */
					size_t count = (size_t)msg.args.arg2;
					uint *data = (uint*)malloc(count);
					msg.args.arg1 = 0;
					if(data) {
						ushort *d = (ushort*)data;
						msg.args.arg1 = count;
						count /= sizeof(ushort);
						while(count-- > 0)
							*d++ = rand();
					}
					msg.args.arg2 = true;
					send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
					if(data) {
						send(fd,MSG_DRV_READ_RESP,data,msg.args.arg1 / sizeof(uint));
						free(data);
					}
				}
				break;
			}
			close(fd);
		}
	}

	/* clean up */
	close(id);
	return EXIT_SUCCESS;
}
