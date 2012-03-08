/**
 * $Id: randmain.c 1091 2011-11-12 22:08:05Z nasmussen $
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
#include <esc/io.h>
#include <esc/mem.h>
#include <esc/messages.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static sMsg msg;

int main(int argc,char **argv) {
	int id;
	char *addr;
	size_t size;

	if(argc != 3)
		error("Usage: %s <wait> <module>",argv[0]);

	id = createdev("/dev/ramdisk",DEV_TYPE_BLOCK,DEV_READ | DEV_WRITE);
	if(id < 0)
		error("Unable to register device 'ramdisk'");

	addr = (char*)mapmod(argv[2],&size);
	if(!addr)
		error("Unable to map module '%s' as ramdisk",argv[2]);

	if(fcntl(id,F_SETDATA,true) < 0)
		error("fcntl");

	/* we're ready now, so create a dummy-vfs-node that tells fs that the disk is registered */
	FILE *f = fopen("/system/devices/disk","w");
	fclose(f);

    /* wait for commands */
	while(1) {
		msgid_t mid;
		int fd = getwork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[RAND] Unable to get work");
		else {
			switch(mid) {
				case MSG_DEV_READ: {
					ulong offset = msg.args.arg1;
					ulong count = msg.args.arg2;
					msg.args.arg1 = 0;
					if(offset + count <= size && offset + count > offset)
						msg.data.arg1 = count;
					msg.args.arg2 = true;
					send(fd,MSG_DEV_READ_RESP,&msg,sizeof(msg.args));
					if(msg.args.arg1 > 0)
						send(fd,MSG_DEV_READ_RESP,addr + offset,msg.data.arg1);
				}
				break;

				case MSG_DEV_WRITE: {
					ulong offset = msg.args.arg1;
					ulong count = msg.args.arg2;
					msg.args.arg1 = 0;
					if(offset + count <= size && offset + count > offset)
						msg.args.arg1 = IGNSIGS(receive(fd,&mid,addr + offset,count));
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_DISK_GETSIZE: {
					msg.args.arg1 = size;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				default:
					msg.args.arg1 = -ENOTSUP;
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
