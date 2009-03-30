/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>
#include <proc.h>
#include <io.h>
#include <bufio.h>
#include <ports.h>
#include <dir.h>
#include <string.h>
#include <service.h>
#include <mem.h>
#include <heap.h>
#include <messages.h>

#include <assert.h>
#include <sllist.h>

#if 0
#define ID_OUT		0
#define ID_IN		1
#define ID_CLEAR	2

static void printProcess(sProc *p) {
	printf("Process:\n");
	printf("\tpid=%d\n",p->pid);
	printf("\tparentPid=%d\n",p->parentPid);
	printf("\tstate=%d\n",p->state);
	printf("\ttextPages=%d\n",p->textPages);
	printf("\tdataPages=%d\n",p->dataPages);
	printf("\tstackPages=%d\n",p->stackPages);
	u32 *ptr = (u32*)(&p->cycleCount);
	printf("\tcycleCount=0x%08x%08x\n",*(ptr + 1),*ptr);
}

static void logChar(char c) {
	outByte(0xe9,c);
	outByte(0x3f8,c);
	while((inByte(0x3fd) & 0x20) == 0);
}
#endif

int main(int argc,char **argv) {
	/*debugf("ret=%d\n",printc('a'));
	debugf("ret=%d\n",printc('b'));
	debugf("ret=%d\n",printc('c'));
	debugf("ret=%d\n",prints("Das ist mein Name\n"));
	debugf("ret=%d\n",printn(1234));
	debugf("ret=%d\n",printn(-1234));
	debugf("ret=%d\n",printn(0));
	debugf("ret=%d\n",printn(100));
	debugf("ret=%d\n",printu(1234,10));
	debugf("ret=%d\n",printu(0,10));
	debugf("ret=%d\n",printu(0xABCDEF,16));
	flush();*/

	exec(0x12345678,NULL);

#if 0
	char str[200] = "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST";
	s32 res;
	u32 max;

	/*sprintf(str,"Huhu, p=%d, s=%x, bla=%s\n",12,0xABC,"string");
	printf("str=%s\n",str);*/

	for(max = 1; max < 31; max++) {
		res = snprintf(str,max,"%s_%d_%x_%o_%b_%u_%c\n","hier",12,0x123,071,0xFF,841,'a');
		printf("[max=%d] res=%d,str='%s'\n",max,res,str);
	}

	u32 m,d,y;
	res = sscanf("12/4/89","%2d/%2d/%2d",&m,&d,&y);
	printf("res=%d, Date: %02d/%02d/%02d\n",res,m,d,y);
#endif

	/*u32 res;
	s32 u,d,e;
	char str[11],ch;

	prints("Enter '%2d/%3d': ");
	flush();
	res = scanf("%2d/%3d",&d,&e);
	printf("res=%d, d=%d, e=%d\n",res,d,e);

	prints("Enter '%b': ");
	flush();
	res = scanf("%b",&u);
	printf("res=%d, u=%d\n",res,u);

	prints("Enter '%x': ");
	flush();
	res = scanf("%x",&u);
	printf("res=%d, u=%d\n",res,u);

	prints("Enter '%o': ");
	flush();
	res = scanf("%o",&u);
	printf("res=%d, u=%d\n",res,u);

	prints("Enter '%c:%10s': ");
	flush();
	res = scanf("%c:%10s",&ch,str);
	printf("res=%d, ch=%d, str=%s\n",res,ch,str);*/

	/*char buffer[12];
	printf("Please enter your name: ");
	scanl(buffer,12);
	printf("Hello %s!\n",buffer);*/

	/*char buffer[32];
	char *str = "Das ist mein String\n";
	s32 stdout,fd = open("system:/pipe",IO_READ | IO_WRITE);
	if(fd < 0) {
		printLastError();
		return 1;
	}

	stdout = dupFd(STDOUT_FILENO);
	redirFd(STDOUT_FILENO,fd);

	u32 i;
	for(i = 0; i < 10; i++) {
		printf("Das ist mein String!!\n");
	}

	redirFd(STDOUT_FILENO,stdout);

	if(fork() == 0) {
		redirFd(STDIN_FILENO,fd);
		s32 c;
		printf("Child reads:\n");
		while((c = scanc()) > 0) {
			printc(c);
		}
		flush();
		printf("Ready\n");
		exit(0);
	}

	wait(EV_CHILD_DIED);
	printf("Ok, closing file\n");
	close(fd);*/

	/*printf("I am task2....yeah!! pid=%d\n",getpid());
	printf("Ok, that's enough for now...\n");
	while(1) {
		yield();
	}*/
	return 0;


#if 0
	if(fork() == 0) {
		s32 id = regService("test",SERVICE_TYPE_MULTIPIPE);

		static sMsgHeader header;
		while(1) {
			s32 cfd = getClient(id);
			if(cfd < 0)
				printLastError();
			else {
				u32 x = 0;
				s32 c = 0;
				do {
					if((c = read(cfd,&header,sizeof(sMsgHeader))) < 0)
						printLastError();
					else if(c > 0) {
						sMsgHeader *msg;
						u8 *data = NULL;
						/* read data */
						if(header.length > 0) {
							data = malloc(header.length * sizeof(u8));
							if(read(cfd,data,header.length) < 0)
								printLastError();
						}

						/* build msg and send */
						msg = asmDataMsg(header.id,header.length,data);
						if(write(fd,msg,sizeof(sMsgHeader) + header.length) < 0)
							printLastError();

						if(header.length > 0)
							free(data);
						x++;
					}
				}
				while(c > 0);
				close(cfd);
			}
		}
		unregService(id);
	}

	s32 fd2;
	do {
		fd2 = open("services:/test",IO_READ | IO_WRITE);
		if(fd2 < 0)
			yield();
	}
	while(fd2 < 0);
	debugf("fd2=%d\n",fd2);

	if(redirFd(fd,fd2) < 0)
		printLastError();
	else
		debugf("Redirected %d to %d\n",fd,fd2);

	char str[] = "Test: \e[30mblack\e[0m, \e[31mred\e[0m, \e[32mgreen\e[0m"
			", \e[33morange\e[0m, \e[34mblue\e[0m, \e[35mmargenta\e[0m, \e[36mcyan\e[0m"
			", \e[37mgray\e[0m\n"
			"\e[30;44mwithbg\e[0m, \e[34;43mwithbg\e[0m\n";
	sMsgHeader *msg = asmDataMsg(MSG_VTERM_WRITE,strlen(str) + 1,str);
	/*u32 i;
	s32 err;
	for(i = 0; i < 3 ;i++) {
		if((err = write(fd,msg,sizeof(sMsgHeader) + msg->length)) < 0) {
			if(err == ERR_NOT_ENOUGH_MEM)
				yield();
			else
				printLastError();
		}
		else {
			yield();
		}
	}*/

	/*u32 i,x;
	for(i = 0,x = 0; x < 10 ;x++) {
		printf("\e[30mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[31mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[32mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[33mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[34mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[35mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[36mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[37mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[40mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[41mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[42mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[43mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[44mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[45mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[46mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[47mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[37;40mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[36;41mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[35;42mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[34;43mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[33;44mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[32;45mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[31;46mDas ist der %dte Test!!\e[0m\n",i++);
		printf("\e[30;47mDas ist der %dte Test!!\e[0m\n",i++);
		volatile u32 x;
		for(x = 0; x < 0xFFFFFFF; x++);
	}*/

	while(1) {
		printf("Type something: ");
		char *buffer = malloc(20 * sizeof(char));
		readLine(buffer,20);
		printf("You typed: %s\n",buffer);
	}

	/*tFD dd,dfd;
	sProc proc;
	sDir *entry;
	char path[] = "system:/processes/";
	char ppath[255];
	while(1) {
		printf("\r");
		if((dd = opendir(path)) >= 0) {
			while((entry = readdir(dd)) != NULL) {
				if(strcmp(entry->name,".") == 0 || strcmp(entry->name,"..") == 0)
					continue;

				strncpy(ppath,path,strlen(path));
				strncat(ppath,entry->name,strlen(entry->name));
				if((dfd = open(ppath,IO_READ)) >= 0) {
					read(dfd,&proc,sizeof(sProc));
					u32 *ptr = &proc.cycleCount;
					printf("%s: %08x%08x ",entry->name,*(ptr + 1),*ptr);
					close(dfd);
				}
			}
			closedir(dd);
		}
	}*/

	/*
	sMsgDataVidGoto vidgoto;
	vidgoto.col = 0;
	vidgoto.row = 10;
	msg = asmDataMsg(MSG_VIDEO_GOTO,sizeof(sMsgDataVidGoto),&vidgoto);
	write(fd,msg,sizeof(sMsgHeader) + msg->length);

	msg = asmDataMsg(MSG_VIDEO_PUTS,5,"Test");
	write(fd,msg,sizeof(sMsgHeader) + msg->length);

	vidgoto.col = 0;
	vidgoto.row = 13;
	msg = asmDataMsg(MSG_VIDEO_GOTO,sizeof(sMsgDataVidGoto),&vidgoto);
	write(fd,msg,sizeof(sMsgHeader) + msg->length);

	msg = asmDataMsg(MSG_VIDEO_PUTS,5,"Test");
	write(fd,msg,sizeof(sMsgHeader) + msg->length);*/

	freeDefMsg(msg);

	/*fork();

	 read from keyboard
	s32 fd3;
	do {
		fd3 = open("services:/keyboard",IO_READ);
		if(fd3 < 0)
			yield();
	}
	while(fd3 < 0);

	static sMsgKbResponse kbRes;

	kbReq.id = MSG_KEYBOARD_READ;
	while(true) {
		if(read(fd3,&kbRes,sizeof(kbRes)) <= 0)
			yield();
		else {
			if(kbRes.isBreak)
				debugf("(pid=%d) Key %d released\n",getpid(),kbRes.keycode);
			else
				debugf("(pid=%d) Key %d pressed\n",getpid(),kbRes.keycode);
		}
	}
	close(fd3);*/


	s32 id = regService("console",SERVICE_TYPE_MULTIPIPE);
	if(id < 0)
		printLastError();
	else {
		if(fork() == 0) {
			s32 sfd = open("services:/console",IO_READ | IO_WRITE);
			if(sfd < 0)
				printLastError();
			else {
				char str[] = "Test";
				sMsgHeader *msg = createMsg(ID_OUT,5,str);
				do {
					if(write(sfd,msg,sizeof(sMsgHeader) + msg->length) < 0)
						printLastError();
					yield();
				}
				while(1);
				freeMsg(msg);
			}
			close(sfd);
			while(1);
		}

		if(fork() == 0) {
			s32 sfd = open("services:/console",IO_READ | IO_WRITE);
			if(sfd < 0)
				printLastError();
			else {
				sMsgHeader *msg = createMsg(ID_CLEAR,0,NULL);
				do {
					if(write(sfd,msg,sizeof(sMsgHeader)) < 0)
						printLastError();
					yield();
				}
				while(1);
				freeMsg(msg);
			}
			close(sfd);
			while(1);
		}

		videoBase = mapPhysical(0xB8000,COLS * ROWS * 2);
		video = videoBase;
		vid_clearScreen();
		vid_setFGColor(WHITE);
		vid_setBGColor(BLACK);

		static sMsgHeader msg;
		do {
			s32 fd = getClient(id);
			if(fd < 0)
				printLastError();
			else {
				read(fd,&msg,sizeof(sMsgHeader));
				if(msg.id == ID_OUT) {
					char *readBuf = malloc(msg.length * sizeof(char));
					read(fd,readBuf,msg.length);
					debugf("Read: __%s__\n",readBuf);
					free(readBuf);
				}
				else if(msg.id == ID_IN) {

				}
				else if(msg.id == ID_CLEAR) {
					/*vid_clearScreen();*/
					debugf("CLEAR\n");
				}
				close(fd);
			}
		}
		while(1);

		unregService();
	}

	s32 id = regService("console",SERVICE_TYPE_MULTIPIPE);
	if(id < 0)
		printLastError();
	else {
		char buf[] = "Das ist ein Teststring\n";
		s32 fd = open("services:/console",IO_READ | IO_WRITE);
		if(fd < 0)
			printLastError();
		else {
			if(fork() == 0) {
				char *readBuf = malloc(strlen(buf) * sizeof(char));
				while(1) {
					if(readBuf == NULL)
						printLastError();
					else {
						if(read(fd,readBuf,strlen(buf)) < 0)
							printLastError();
						else {
							debugf("Read: %s\n",readBuf);
							/* reset buffer */
							*readBuf = '\0';
						}
					}
					yield();
				}
				close(fd);
			}
			else {
				while(1) {
					if(write(fd,buf,strlen(buf)) < 0)
						printLastError();
					else {
						debugf("Wrote: %s\n",buf);
					}
					yield();
				}
			}
		}

		unregService();
	}


	s32 id = regService("console",SERVICE_TYPE_MULTIPIPE);
	if(id < 0)
		printLastError();
	else {
		debugf("Registered service\n");

		if(fork() == 0) {
			debugf("mypid=%d, ppid=%d\n",getpid(),getppid());
			char buf[100];
			s32 sfd = open("services:/console",IO_READ | IO_WRITE);
			debugf("Opened service with fd %d\n",sfd);
			yield();
			if(read(sfd,buf,100) < 0)
				printLastError();
			else
				debugf("Read: %s\n",buf);
			close(sfd);
			while(1);
		}

		yield();
		if(unregService(id) < 0)
			printLastError();
		else
			debugf("Unregistered service\n");
	}

	/*videoBase = mapPhysical(0xB8000,COLS * ROWS * 2);
	debugf("videoBase=0x%x\n",videoBase);
	if(videoBase == NULL) {
		printLastError();
	}
	else {
		video = videoBase;
		vid_clearScreen();
		vid_setFGColor(WHITE);
		vid_setBGColor(BLACK);

		vid_printf("Hallo, ich bins :P\n");
	}

	sSLList *l = sll_create();
	sll_append(l,1);
	sll_append(l,2);
	sll_append(l,3);

	sll_dbg_print(l);

	sll_destroy(l,false);*/

	/*test_register(&tModHeap);
	test_start();*/

	/*s32 count;

	count = changeSize(3);
	if(count < 0)
		printLastError();
	else
		debugf("dataPages = %d\n",count);

	count = changeSize(-3);
	if(count < 0)
		printLastError();
	else
		debugf("dataPages = %d\n",count);

	u32 *ptr[10],*p,*first;
	u32 c,i,x = 0;

	c = 10;
	ptr[x] = (u32*)malloc(sizeof(u32) * c);
	first = ptr[x];
	debugf("ptr=0x%x\n",ptr[x]);
	p = ptr[x];
	for(i = 0;i < c;i++)
		*p++ = x + 1;

	x++;
	c = 3;
	ptr[x] = (u32*)malloc(sizeof(u32) * c);
	debugf("ptr=0x%x\n",ptr[x]);
	p = ptr[x];
	for(i = 0;i < c;i++)
		*p++ = x + 1;

	x++;
	c = 2;
	ptr[x] = (u32*)malloc(sizeof(u32) * c);
	debugf("ptr=0x%x\n",ptr[x]);
	p = ptr[x];
	for(i = 0;i < c;i++)
		*p++ = x + 1;

	x++;
	c = 10;
	ptr[x] = (u32*)malloc(sizeof(u32) * c);
	debugf("ptr=0x%x\n",ptr[x]);
	p = ptr[x];
	for(i = 0;i < c;i++)
		*p++ = x + 1;

	free(ptr[2]);
	free(ptr[0]);
	free(ptr[1]);

	for(i = 0;i < 25;i++) {
		debugf("0x%x = 0x%x\n",first,*first);
		first++;
	}

	ptr = (u32*)malloc(sizeof(u32) * 1025);
	debugf("ptr=0x%x\n",ptr);

	debugf("ptr=0x%x\n",malloc(4092));
	debugf("ptr=0x%x\n",malloc(3996));
	debugf("ptr=0x%x\n",malloc(10));

	printHeap();

	u16 pid = fork();
	if(pid == 0) {
		debugf("I am the child with pid %d\n",getpid());
	}
	else if(pid == -1) {
		debugf("Fork failed\n");
	}
	else {
		debugf("I am the parent, created child %d\n",pid);
	}

	while(true) {
		debugf("Hi, my pid is %d, parent is %d\n",getpid(),getppid());
	}

	if(regService("test",SERVICE_TYPE_MULTIPIPE) < 0)
		printLastError();
	else
		unregService();

	s32 dd;
	sDir *entry;
	debugf("Listing directory '/':\n");
	if((dd = opendir("/")) >= 0) {
		while((entry = readdir(dd)) != NULL) {
			debugf("- 0x%x : %s\n",entry->nodeNo,entry->name);
		}
		closedir(dd);
	}

	if(fork() == 0) {
		tFD fd;
		sProc proc;
		char path[] = "system:/processes/";
		char ppath[255];
		debugf("(%d) Listing all processes:\n",getpid());
		if((dd = opendir(path)) >= 0) {
			while((entry = readdir(dd)) != NULL) {
				strncpy(ppath,path,strlen(path));
				strncat(ppath,entry->name,strlen(entry->name));
				if((fd = open(ppath,IO_READ)) >= 0) {
					read(fd,&proc,sizeof(sProc));
					printProcess(&proc);
					close(fd);
				}
			}
			closedir(dd);
		}
		return 0;
	}*/

	while(1);
	return 0;
#endif
}
