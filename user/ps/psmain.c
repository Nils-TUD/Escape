/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/dir.h>
#include <esc/proc.h>
#include <string.h>

/**
 * Prints the given process
 *
 * @param p the process
 */
static void ps_printProcess(sProc *p);

static const char *states[] = {
	"Unused ",
	"Running",
	"Ready  ",
	"Blocked",
	"Zombie "
};

int main(void) {
	tFD dd,dfd;
	sProc proc;
	sDirEntry *entry;
	char path[] = "system:/processes/";
	char ppath[255];

	printf("PID\tPPID\tPAGES\tSTATE\t\tCYCLES\t\t\t\tCOMMAND\n");

	if((dd = opendir(path)) >= 0) {
		while((entry = readdir(dd)) != NULL) {
			if(strcmp(entry->name,".") == 0 || strcmp(entry->name,"..") == 0)
				continue;

			strcpy(ppath,path);
			strncat(ppath,entry->name,strlen(entry->name));
			if((dfd = open(ppath,IO_READ)) >= 0) {
				read(dfd,&proc,sizeof(sProc));
				ps_printProcess(&proc);
				close(dfd);
			}
			else {
				printf("Unable to open '%s'\n",ppath);
				return 1;
			}
		}
		closedir(dd);
	}
	else {
		printf("Unable to open '%s'\n",path);
		return 1;
	}

	printf("\n");

	return 0;
}

static void ps_printProcess(sProc *p) {
	u32 *ptr = (u32*)&p->cycleCount;
	printf("%2d\t%2d\t\t%3d\t\t%s\t\t%#08x%08x\t%s\n",
			p->pid,p->parentPid,p->textPages + p->dataPages + p->stackPages,
			states[p->state],*(ptr + 1),*ptr,p->command);
}
