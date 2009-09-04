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
#include <esc/fileio.h>
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/service.h>
#include <esc/date.h>
#include <stdlib.h>
#include <string.h>
#include <messages.h>

#define BUF_SIZE 4096 * 32
/*#define BUF_SIZE 1024*/
#define COUNT 18000

static u8 buffer[BUF_SIZE];

int main(int argc,char *argv[]) {
	/*printf("I am evil ^^\n");
	debugChar(0x12345678);
	open(0x123456578,IO_READ);
	u32 *ptr = (u32*)0xFFFFFFFF;
	*ptr = 1;
	printf("Never printed\n");*/
#if 1
	tFD fd;
	u64 start;
	uLongLong total;
	u64 i,diff,t;
	bool disk = false;
	const char *path = "/system/test";

	if(argc > 1) {
		path = argv[1];
		disk = true;
	}

	fd = open(path,IO_READ | IO_WRITE | IO_CREATE);
	if(fd < 0) {
		printe("open");
		return EXIT_FAILURE;
	}

	printf("Testing speed of %s '%s'\n",disk ? "read from" : "read/write from/to",path);
	printf("Transferring %u MiB in chunks of %d bytes\n",(u32)(((u64)COUNT * BUF_SIZE) / M),BUF_SIZE);
	printf("\n");

	if(!disk) {
		t = getTime();
		start = cpu_rdtsc();
		for(i = 0; i < COUNT; i++) {
			if(write(fd,buffer,sizeof(buffer)) < 0) {
				printe("write");
				break;
			}
			if(seek(fd,0,SEEK_SET) < 0) {
				printe("seek");
				break;
			}
			if(i % (COUNT / 1000) == 0) {
				diff = getTime() - t;
				printf("\rWriting with	%04d MiB/s",diff == 0 ? 0 : ((i * sizeof(buffer) / diff) / M));
			}
		}

		total.val64 = cpu_rdtsc() - start;
		diff = getTime() - t;
		printf("\n");
		printf("Instructions:	%08x%08x\n",total.val32.upper,total.val32.lower);
		printf("Speed:			%04d MiB/s\n",diff == 0 ? 0 : ((i * sizeof(buffer) / diff) / M));
		printf("\n");
	}

	t = getTime();
	start = cpu_rdtsc();
	for(i = 0; i < COUNT; i++) {
		if(read(fd,buffer,sizeof(buffer)) < 0) {
			printe("read");
			break;
		}
		if(seek(fd,0,SEEK_SET) < 0) {
			printe("seek");
			break;
		}
		if(i % (COUNT / 1000) == 0) {
			diff = getTime() - t;
			printf("\rReading with	%04d MiB/s",diff == 0 ? 0 : ((i * sizeof(buffer) / diff) / M));
		}
	}

	total.val64 = cpu_rdtsc() - start;
	diff = getTime() - t;
	printf("\n");
	printf("Instructions:	%08x%08x\n",total.val32.upper,total.val32.lower);
	printf("Speed:			%04d MiB/s\n",diff == 0 ? 0 : ((i * sizeof(buffer) / diff) / M));

	close(fd);

	/*int c1,c2,c3;
	char line[50];
	char str[] = "- This, a sample string.";
	char *pch;
	s32 res;

	printf("Splitting string \"%s\" into tokens:\n",str);
	pch = strtok(str," ,.-");
	while(pch != NULL) {
		printf("'%s'\n",pch);
		pch = strtok(NULL," ,.-");
	}

	printf("str=%p,%n pch=%p,%n abc=%p,%n\n",str,&c1,pch,&c2,0x1234,&c3);
	printf("c1=%d, c2=%d, c3=%d\n",c1,c2,c3);

	printf("num1: '%-8d', num2: '%8d', num3='%-16x', num4='%-012X'\n",
			100,200,0x1bca,0x12345678);

	printf("num1: '%-+4d', num2: '%-+4d'\n",-100,50);
	printf("num1: '%- 4d', num2: '%- 4d'\n",-100,50);
	printf("num1: '%#4x', num2: '%#4X', num3 = '%#4o'\n",0x123,0x456,0377);
	printf("Var padding: %*d\n",8,-123);
	printf("short: %4hx\n",0x1234);

	while(1) {
		printf("Now lets execute something...\n");
		scanl(line,50);
		res = system(line);
		printf("Result: %d\n",res);
	}*/
#endif

#if 0
	tServ client;
	tServ id = regService("bla",SERV_DRIVER);
	if(id < 0)
		printe("regService");

	if(fork() == 0) {
		char buf[10] = {0};
		tFD fd = open("/drivers/bla",IO_READ | IO_WRITE | IO_CONNECT);
		if(fd < 0)
			printe("open");
		printf("Reading...\n");
		read(fd,buf,10);
		printf("Res: %s\n",buf);
		printf("Writing %s...\n",buf);
		write(fd,buf,10);
		printf("IOCtl read\n");
		ioctl(fd,1,(u8*)buf,10);
		printf("Got '%s'\n",buf);
		printf("IOCtl write\n");
		ioctl(fd,0,(u8*)buf,10);
		printf("Closing...\n");
		close(fd);
		return EXIT_SUCCESS;
	}

	setDataReadable(id,true);
	bool quit = false;
	tMsgId mid;
	static sMsg msg;
	while(!quit) {
		tFD cfd = getClient(&id,1,&client);
		if(cfd < 0)
			wait(EV_CLIENT | EV_RECEIVED_MSG);
		else {
			while(receive(cfd,&mid,&msg,sizeof(msg)) > 0) {
				switch(mid) {
					case MSG_DRV_OPEN:
						printf("Open: tid=%d, flags=%d\n",msg.args.arg1,msg.args.arg2);
						msg.args.arg1 = 0;
						send(cfd,MSG_DRV_OPEN_RESP,&msg,sizeof(msg.args));
						break;
					case MSG_DRV_READ:
						printf("Read: tid=%d, offset=%d, count=%d\n",msg.args.arg1,
								msg.args.arg2,msg.args.arg3);
						msg.data.arg1 = msg.args.arg1;
						msg.data.arg2 = msg.args.arg3;
						msg.data.arg3 = true;
						strcpy(msg.data.d,"test!!");
						send(cfd,MSG_DRV_READ_RESP,&msg,sizeof(msg.data));
						break;
					case MSG_DRV_WRITE:
						printf("Write: tid=%d, offset=%d, count=%d\n",msg.data.arg1,
								msg.data.arg2,msg.data.arg3);
						printf("Got %s\n",msg.data.d);
						msg.args.arg1 = msg.data.arg1;
						msg.args.arg2 = msg.data.arg3;
						send(cfd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
						break;
					case MSG_DRV_IOCTL: {
						u32 cmd = msg.data.arg2;
						printf("Ioctl: tid=%d, cmd=%d, dsize=%d\n",msg.data.arg1,
								msg.data.arg2,msg.data.arg3);
						msg.data.arg2 = 0;
						if(cmd == 0) {
							msg.data.arg3 = 0;
							printf("Got '%s'\n",msg.data.d);
						}
						else {
							msg.data.arg3 = 9;
							strcpy(msg.data.d,"test1234");
						}
						send(cfd,MSG_DRV_IOCTL_RESP,&msg,sizeof(msg.data));
					}
					break;
					case MSG_DRV_CLOSE:
						printf("Close\n");
						quit = true;
						break;
					default:
						printf("Unknown command");
						break;
				}
			}
			close(cfd);
		}
	}

	unregService(id);
#endif

#if 0
	debug();
#endif

	return EXIT_SUCCESS;
}
