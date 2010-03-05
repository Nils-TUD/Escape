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
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/dir.h>
#include <esc/proc.h>
#include <esc/cmdargs.h>
#include <esc/heap.h>
#include <esc/algo.h>
#include <sllist.h>
#include <width.h>
#include <string.h>

#define ARRAY_INC_SIZE		10
#define MAX_SORTNAME_LEN	16

#define SORT_PID			0
#define SORT_PPID			1
#define SORT_MEM			2
#define SORT_CPU			3
#define SORT_UCPU			4
#define SORT_KCPU			5
#define SORT_NAME			6

typedef struct {
	u8 type;
	char name[MAX_SORTNAME_LEN + 1];
} sSort;

/* process-data */
typedef struct {
	tPid pid;
	tPid parentPid;
	u32 frames;
	u32 textPages;
	u32 dataPages;
	u32 stackPages;
	uLongLong ucycleCount;
	uLongLong kcycleCount;
	sSLList *threads;
	char command[MAX_PROC_NAME_LEN + 1];
} sProcess;

typedef struct {
	tTid tid;
	tPid pid;
	u8 state;
	u32 stackPages;
	uLongLong ucycleCount;
	uLongLong kcycleCount;
} sPThread;

static int cmpulongs(u64 a,u64 b);
static int compareProcs(const void *a,const void *b);
static sProcess *ps_getProcs(u32 *count);
static bool ps_readProc(tFD fd,tPid pid,sProcess *p);
static bool ps_readThread(tFD fd,sPThread *t);
static char *ps_readNode(tFD fd);

static sSort sorts[] = {
	{SORT_PID,"pid"},
	{SORT_PPID,"ppid"},
	{SORT_MEM,"mem"},
	{SORT_CPU,"cpu"},
	{SORT_UCPU,"ucpu"},
	{SORT_KCPU,"kcpu"},
	{SORT_NAME,"name"},
};

static const char *states[] = {
	"Unused ",
	"Running",
	"Ready  ",
	"Blocked",
	"Zombie "
};
static u8 sort = SORT_PID;

static void usage(char *name) {
	u32 i;
	fprintf(stderr,"Usage: %s [-t][-n <count>][-s <sort>]\n",name);
	fprintf(stderr,"	-t			: Print threads, too\n");
	fprintf(stderr,"	-n <count>	: Print first <count> processes\n");
	fprintf(stderr,"	-s <sort>	: Sort by ");
	for(i = 0; i < ARRAY_SIZE(sorts); i++) {
		fprintf(stderr,"'%s'",sorts[i].name);
		if(i < ARRAY_SIZE(sorts) - 1)
			fprintf(stderr,", ");
	}
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	sProcess *procs;
	u32 i,j,count;
	u64 totalCycles;
	sSLNode *n;
	u32 numProcs = 0;
	bool printThreads = false;
	char *s;

	if(isHelpCmd(argc,argv))
		usage(argv[0]);

	for(i = 1; (int)i < argc; i++) {
		if(strcmp(argv[i],"-s") == 0) {
			if((s32)i >= argc - 1)
				usage(argv[0]);
			i++;
			for(j = 0; j < ARRAY_SIZE(sorts); j++) {
				if(strcmp(argv[i],sorts[j].name) == 0) {
					sort = sorts[j].type;
					break;
				}
			}
		}
		else if(strcmp(argv[i],"-n") == 0) {
			if((s32)i >= argc - 1)
				usage(argv[0]);
			i++;
			numProcs = atoi(argv[i]);
		}
		else if(argv[i][0] == '-') {
			s = argv[i] + 1;
			while(*s) {
				switch(*s) {
					case 't':
						printThreads = true;
						break;
					default:
						usage(argv[0]);
				}
				s++;
			}
		}
		else
			usage(argv[0]);
	}

	procs = ps_getProcs(&count);
	if(procs == NULL)
		return EXIT_FAILURE;

	/* sum total cycles and assign cycles to processes */
	totalCycles = 0;
	for(i = 0; i < count; i++) {
		u64 procUCycles = 0,procKCycles = 0;
		for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
			sPThread *t = (sPThread*)n->data;
			totalCycles += t->ucycleCount.val64 + t->kcycleCount.val64;
			procUCycles += t->ucycleCount.val64;
			procKCycles += t->kcycleCount.val64;
		}
		procs[i].ucycleCount.val64 = procUCycles;
		procs[i].kcycleCount.val64 = procKCycles;
	}

	/* TODO it would be better to store pointers so that we don't have to swap the whole processes,
	 * right? */
	qsort(procs,count,sizeof(sProcess),compareProcs);

	if(numProcs == 0)
		numProcs = count;

	/* determine max-values (we want to have a min-width here :)) */
	u32 maxPid = 100;
	u32 maxPpid = 100;
	u32 maxPmem = 10000;
	u32 maxVmem = 10000;
	for(i = 0; i < numProcs; i++) {
		if(procs[i].pid > maxPid)
			maxPid = procs[i].pid;
		if(procs[i].parentPid > maxPpid)
			maxPpid = procs[i].parentPid;
		if(procs[i].frames > maxPmem)
			maxPmem = procs[i].frames;
		if(procs[i].textPages + procs[i].dataPages + procs[i].stackPages > maxVmem)
			maxVmem = procs[i].textPages + procs[i].dataPages + procs[i].stackPages;
		if(printThreads) {
			for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
				sPThread *t = (sPThread*)n->data;
				if(t->tid > maxPpid)
					maxPpid = t->tid;
			}
		}
	}
	maxPid = getuwidth(maxPid,10);
	maxPpid = getuwidth(maxPpid,10);
	maxPmem = getuwidth(maxPmem * 4,10);
	maxVmem = getuwidth(maxVmem * 4,10);

	/* now print processes */
	printf("%*sPID%*sPPID%*sPMEM%*sVMEM  STATE    %%CPU (USER,KERNEL) COMMAND\n",
			maxPid - 3,"",maxPpid - 1,"",maxPmem + 1,"",maxVmem + 1,"");

	for(i = 0; i < numProcs; i++) {
		u64 procCycles;
		u32 userPercent,kernelPercent;
		float cyclePercent;
		procCycles = procs[i].ucycleCount.val64 + procs[i].kcycleCount.val64;
		cyclePercent = (float)(100. / (totalCycles / (double)procCycles));
		userPercent = (u32)(100. / (procCycles / (double)procs[i].ucycleCount.val64));
		kernelPercent = (u32)(100. / (procCycles / (double)procs[i].kcycleCount.val64));
		printf("%*u   %*u %*u KiB %*u KiB  -       %4.1f%% (%3d%%,%3d%%)   %s\n",
				maxPid,procs[i].pid,maxPpid,procs[i].parentPid,maxPmem,procs[i].frames * 4,
				maxVmem,(procs[i].textPages + procs[i].dataPages + procs[i].stackPages) * 4,
				cyclePercent,userPercent,kernelPercent,procs[i].command);

		if(printThreads) {
			for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
				sPThread *t = (sPThread*)n->data;
				u64 threadCycles = t->ucycleCount.val64 + t->kcycleCount.val64;
				float tcyclePercent = (float)(100. / (totalCycles / (double)threadCycles));
				u32 tuserPercent = (u32)(100. / (threadCycles / (double)t->ucycleCount.val64));
				u32 tkernelPercent = (u32)(100. / (threadCycles / (double)t->kcycleCount.val64));
				printf("  %c\xC4%*s%*d%*s%s %4.1f%% (%3d%%,%3d%%)\n",
						n->next == NULL ? 0xC0 : 0xC3,
						maxPid - 3,"",maxPpid,t->tid,14 + maxPmem + maxVmem,"",states[t->state],tcyclePercent,
						tuserPercent,tkernelPercent);
			}
		}
	}

	printf("\n");

	for(i = 0; i < count; i++)
		sll_destroy(procs[i].threads,true);
	free(procs);
	return EXIT_SUCCESS;
}

static int compareProcs(const void *a,const void *b) {
	sProcess *p1 = (sProcess*)a;
	sProcess *p2 = (sProcess*)b;
	switch(sort) {
		case SORT_PID:
			/* ascending */
			return p1->pid - p2->pid;
		case SORT_PPID:
			/* ascending */
			return p1->parentPid - p2->parentPid;
		case SORT_MEM:
			/* descending */
			return (p2->textPages + p2->dataPages + p2->stackPages) -
				(p1->textPages + p1->dataPages + p1->stackPages);
		case SORT_CPU:
			/* descending */
			return cmpulongs(p2->ucycleCount.val64 + p2->kcycleCount.val64,
					p1->ucycleCount.val64 + p1->kcycleCount.val64);
		case SORT_UCPU:
			/* descending */
			return cmpulongs(p2->ucycleCount.val64,p1->ucycleCount.val64);
		case SORT_KCPU:
			/* descending */
			return cmpulongs(p2->kcycleCount.val64,p1->kcycleCount.val64);
		case SORT_NAME:
			/* ascending */
			return strcmp(p1->command,p2->command);
	}
	/* never reached */
	return 0;
}

static int cmpulongs(u64 a,u64 b) {
	if(a > b)
		return 1;
	if(b > a)
		return -1;
	return 0;
}

static sProcess *ps_getProcs(u32 *count) {
	tFD dd,dfd;
	sDirEntry entry;
	char ppath[MAX_PATH_LEN + 1];
	u32 pos = 0;
	u32 size = ARRAY_INC_SIZE;
	sProcess *procs = (sProcess*)malloc(size * sizeof(sProcess));
	if(procs == NULL) {
		printe("Unable to allocate mem for processes");
		return NULL;
	}

	if((dd = opendir("/system/processes")) >= 0) {
		while(readdir(&entry,dd)) {
			if(strcmp(entry.name,".") == 0 || strcmp(entry.name,"..") == 0)
				continue;

			/* build path */
			snprintf(ppath,sizeof(ppath),"/system/processes/%s/info",entry.name);
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
				if(!ps_readProc(dfd,atoi(entry.name),procs + pos)) {
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
				printe("Unable to open '%s'",ppath);
				return NULL;
			}
		}
		closedir(dd);
	}
	else {
		free(procs);
		printe("Unable to open '%s'","/system/processes");
		return NULL;
	}

	*count = pos;
	return procs;
}

static bool ps_readProc(tFD fd,tPid pid,sProcess *p) {
	sDirEntry entry;
	char path[MAX_PATH_LEN + 1];
	char ppath[MAX_PATH_LEN + 1];
	tFD threads,dfd;
	char *buf;

	buf = ps_readNode(fd);
	if(buf == NULL)
		return false;

	/* parse string */
	sscanf(
		buf,
		"%*s%hu" "%*s%hu" "%*s%s" "%*s%u" "%*s%u" "%*s%u" "%*s%u",
		&p->pid,&p->parentPid,&p->command,&p->frames,&p->textPages,
		&p->dataPages,&p->stackPages
	);
	p->threads = sll_create();

	/* read threads */
	snprintf(path,sizeof(path),"/system/processes/%d/threads",pid);
	threads = opendir(path);
	if(threads < 0) {
		free(buf);
		sll_destroy(p->threads,true);
		return false;
	}
	while(readdir(&entry,threads)) {
		if(strcmp(entry.name,".") == 0 || strcmp(entry.name,"..") == 0)
			continue;

		/* build path */
		snprintf(ppath,sizeof(ppath),"/system/processes/%d/threads/%s",pid,entry.name);
		if((dfd = open(ppath,IO_READ)) >= 0) {
			sPThread *t = (sPThread*)malloc(sizeof(sPThread));
			/* read thread */
			if(t == NULL || !ps_readThread(dfd,t)) {
				free(t);
				close(dfd);
				free(buf);
				sll_destroy(p->threads,true);
				printe("Unable to read thread-data");
				return false;
			}

			sll_append(p->threads,t);
			close(dfd);
		}
		else {
			free(buf);
			sll_destroy(p->threads,true);
			printe("Unable to open '%s'",ppath);
			return false;
		}
	}
	closedir(threads);

	free(buf);
	return true;
}

static bool ps_readThread(tFD fd,sPThread *t) {
	u16 state;
	char *buf = ps_readNode(fd);
	if(buf == NULL)
		return false;

	/* parse string; use separate u16 storage for state since we can't tell scanf that is a byte */
	sscanf(
		buf,
		"%*s%hu" "%*s%hu" "%*s%hu" "%*s%u" "%*s%8x%8x" "%*s%8x%8x",
		&t->tid,&t->pid,&state,&t->stackPages,
		&t->ucycleCount.val32.upper,&t->ucycleCount.val32.lower,
		&t->kcycleCount.val32.upper,&t->kcycleCount.val32.lower
	);
	t->state = state;

	free(buf);
	return true;
}

static char *ps_readNode(tFD fd) {
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
	return buf;
}
