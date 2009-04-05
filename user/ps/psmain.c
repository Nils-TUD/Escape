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
#include <stdlib.h>
#include <string.h>

#define ARRAY_INC_SIZE		10

static sProc *ps_getProcs(u32 *count);

static const char *states[] = {
	"Unused ",
	"Running",
	"Ready  ",
	"Blocked",
	"Zombie "
};

int main(void) {
	sProc *procs;
	u32 i,count;
	u64 totalCycles;

	procs = ps_getProcs(&count);
	if(procs == NULL)
		return EXIT_FAILURE;

	/* sum total cycles */
	totalCycles = 0;
	for(i = 0; i < count; i++)
		totalCycles += procs[i].ucycleCount + procs[i].kcycleCount;

	/* now print processes */
	printf("PID\tPPID MEM\t\tSTATE\t%%CPU (USER,KERNEL)\tCOMMAND\n");

	for(i = 0; i < count; i++) {
		u64 procCycles = procs[i].ucycleCount + procs[i].kcycleCount;
		u32 cyclePercent = (u32)(100. / (totalCycles / (double)procCycles));
		u32 userPercent = (u32)(100. / (procCycles / (double)procs[i].ucycleCount));
		u32 kernelPercent = (u32)(100. / (procCycles / (double)procs[i].kcycleCount));
		printf("%2d\t%2d\t%4d KiB\t%s\t%3d%% (%3d%%,%3d%%)\t%s\n",
				procs[i].pid,procs[i].parentPid,(procs[i].textPages + procs[i].dataPages + procs[i].stackPages) * 4,
				states[procs[i].state],cyclePercent,userPercent,kernelPercent,procs[i].command);
	}

	printf("\n");

	free(procs);
	return EXIT_SUCCESS;
}

static sProc *ps_getProcs(u32 *count) {
	tFD dd,dfd;
	sDirEntry *entry;
	char path[] = "system:/processes/";
	char ppath[255];
	u32 pos = 0;
	u32 size = ARRAY_INC_SIZE;
	sProc *procs = (sProc*)malloc(size * sizeof(sProc));
	if(procs == NULL) {
		printe("Unable to allocate mem for processes");
		return NULL;
	}

	if((dd = opendir(path)) >= 0) {
		while((entry = readdir(dd)) != NULL) {
			if(strcmp(entry->name,".") == 0 || strcmp(entry->name,"..") == 0)
				continue;

			strcpy(ppath,path);
			strncat(ppath,entry->name,strlen(entry->name));
			if((dfd = open(ppath,IO_READ)) >= 0) {
				if(pos >= size) {
					size += ARRAY_INC_SIZE;
					procs = (sProc*)realloc(procs,size * sizeof(sProc));
					if(procs == NULL) {
						printe("Unable to allocate mem for processes");
						return NULL;
					}
				}

				if(read(dfd,procs + pos,sizeof(sProc)) < 0) {
					free(procs);
					printe("Unable to read process-data");
					return NULL;
				}

				pos++;
				close(dfd);
			}
			else {
				free(procs);
				printe("Unable to open '%s'\n",ppath);
				return NULL;
			}
		}
		closedir(dd);
	}
	else {
		free(procs);
		printe("Unable to open '%s'\n",path);
		return NULL;
	}

	*count = pos;
	return procs;
}
