/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <errno.h>

void handleFileRead(int fd,sMsg *msg,fGetData getData) {
	size_t total;
	char *data;
	off_t offset = msg->args.arg1;
	size_t count = msg->args.arg2;
	FILE *str;
	if(offset + (off_t)count < offset)
		msg->args.arg1 = -EINVAL;
	str = ascreate();
	if(str)
		getData(str);
	data = asget(str,&total);
	if(!data)
		msg->args.arg1 = -ENOMEM;
	else {
		msg->args.arg1 = 0;
		if(offset >= (off_t)total)
			msg->args.arg1 = 0;
		else if(offset + count > total)
			msg->args.arg1 = total - offset;
		else
			msg->args.arg1 = count;
	}
	send(fd,MSG_DEV_READ_RESP,msg,sizeof(msg->args));
	if((long)msg->args.arg1 > 0)
		send(fd,MSG_DEV_READ_RESP,data + offset,msg->args.arg1);
	fclose(str);
}
