/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>
#include <proc.h>
#include <io.h>
#include <dir.h>
#include <string.h>

static void printProcess(sProc *p);

s32 main(void) {
	/*u16 pid = fork();
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
	}*/

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
		s8 path[] = "/system/processes/";
		s8 ppath[255];
		debugf("Listing all processes:\n");
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
	}

	while(1);
	return 0;
}

static void printProcess(sProc *p) {
	debugf("Process:\n");
	debugf("\tpid=%d\n",p->pid);
	debugf("\tparentPid=%d\n",p->parentPid);
	debugf("\tstate=%d\n",p->state);
	debugf("\ttextPages=%d\n",p->textPages);
	debugf("\tdataPages=%d\n",p->dataPages);
	debugf("\tstackPages=%d\n",p->stackPages);
}
