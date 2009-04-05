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

/* process-data */
typedef struct {
	u8 state;
	tPid pid;
	tPid parentPid;
	u32 textPages;
	u32 dataPages;
	u32 stackPages;
	u64 ucycleCount;
	u64 kcycleCount;
	char command[MAX_PROC_NAME_LEN + 1];
} sProcess;

static sProcess *ps_getProcs(u32 *count);
static char *ps_readProc(tFD fd,sProcess *p);

static const char *states[] = {
	"Unused ",
	"Running",
	"Ready  ",
	"Blocked",
	"Zombie "
};

int main(void) {
	sProcess *procs;
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
				procs[i].pid,procs[i].parentPid,
				(procs[i].textPages + procs[i].dataPages + procs[i].stackPages) * 4,
				states[procs[i].state],cyclePercent,userPercent,kernelPercent,procs[i].command);
	}

	printf("\n");

	free(procs);
	return EXIT_SUCCESS;
}

static sProcess *ps_getProcs(u32 *count) {
	tFD dd,dfd;
	sDirEntry *entry;
	char path[] = "system:/processes/";
	char ppath[255];
	char *sproc;
	u32 pos = 0;
	u32 size = ARRAY_INC_SIZE;
	sProcess *procs = (sProcess*)malloc(size * sizeof(sProcess));
	if(procs == NULL) {
		printe("Unable to allocate mem for processes");
		return NULL;
	}

	if((dd = opendir(path)) >= 0) {
		while((entry = readdir(dd)) != NULL) {
			if(strcmp(entry->name,".") == 0 || strcmp(entry->name,"..") == 0)
				continue;

			/* build path */
			strcpy(ppath,path);
			strncat(ppath,entry->name,strlen(entry->name));
			if((dfd = open(ppath,IO_READ)) >= 0) {
				/* increase array */
				if(pos >= size) {
					size += ARRAY_INC_SIZE;
					procs = (sProcess*)realloc(procs,size * sizeof(sProcess));
					if(procs == NULL) {
						printe("Unable to allocate mem for processes");
						return NULL;
					}
				}

				/* read process */
				sproc = ps_readProc(dfd,procs + pos);
				if(sproc == NULL) {
					close(dfd);
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

static char *ps_readProc(tFD fd,sProcess *p) {
	u32 pos = 0;
	u32 size = 256;
	s32 res;
	char *buf = (char*)malloc(size);
	if(buf == NULL)
		return NULL;

	while((res = read(fd,buf + pos,size - pos)) > 0) {
		pos += res;
		size += 256;
		buf = (char*)realloc(buf,size);
		if(buf == NULL)
			return NULL;
	}

	/* parse string; use separate u16 storage for state since we can't tell scanf that is a byte */
	u16 state;
	sscanf(
		buf,
		"%*s%hu" "%*s%hu" "%*s%s" "%*s%hu" "%*s%u" "%*s%u" "%*s%u" "%*s%8x%x" "%*s%8x%8x",
		&p->pid,&p->parentPid,&p->command,&state,&p->textPages,&p->dataPages,&p->stackPages,
		(u32*)&p->ucycleCount + 1,(u32*)&p->ucycleCount,
		(u32*)&p->kcycleCount + 1,(u32*)&p->kcycleCount
	);
	p->state = state;

	return buf;
}
