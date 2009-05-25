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
#include <stdlib.h>
#include <sllist.h>
#include <string.h>

#define ARRAY_INC_SIZE		10

/* process-data */
typedef struct {
	tPid pid;
	tPid parentPid;
	u32 textPages;
	u32 dataPages;
	u32 stackPages;
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

static sProcess *ps_getProcs(u32 *count);
static bool ps_readProc(tFD fd,tPid pid,sProcess *p);
static bool ps_readThread(tFD fd,sPThread *t);
static char *ps_readNode(tFD fd);

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
	sSLNode *n;

	procs = ps_getProcs(&count);
	if(procs == NULL)
		return EXIT_FAILURE;

	/* sum total cycles */
	totalCycles = 0;
	for(i = 0; i < count; i++) {
		for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
			sPThread *t = (sPThread*)n->data;
			totalCycles += t->ucycleCount.val64 + t->kcycleCount.val64;
		}
	}

	/* now print processes */
	printf("ID\t\tPPID MEM\t\tSTATE\t%%CPU (USER,KERNEL)\tCOMMAND\n");

	for(i = 0; i < count; i++) {
		u64 procUCycles = 0,procKCycles = 0;
		for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
			sPThread *t = (sPThread*)n->data;
			procUCycles += t->ucycleCount.val64;
			procKCycles += t->kcycleCount.val64;
		}
		u64 procCycles = procUCycles + procKCycles;
		u32 cyclePercent = (u32)(100. / (totalCycles / (double)procCycles));
		u32 userPercent = (u32)(100. / (procCycles / (double)procUCycles));
		u32 kernelPercent = (u32)(100. / (procCycles / (double)procKCycles));
		printf("%2d\t\t%2d\t%4d KiB\t-\t\t%3d%% (%3d%%,%3d%%)\t%s\n",
				procs[i].pid,procs[i].parentPid,
				(procs[i].textPages + procs[i].dataPages + procs[i].stackPages) * 4,
				cyclePercent,userPercent,kernelPercent,procs[i].command);

		for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
			sPThread *t = (sPThread*)n->data;
			u64 threadCycles = t->ucycleCount.val64 + t->kcycleCount.val64;
			u32 tcyclePercent = (u32)(100. / (totalCycles / (double)threadCycles));
			u32 tuserPercent = (u32)(100. / (threadCycles / (double)t->ucycleCount.val64));
			u32 tkernelPercent = (u32)(100. / (threadCycles / (double)t->kcycleCount.val64));
			printf(" |-%2d\t\t\t\t\t%s\t%3d%% (%3d%%,%3d%%)\n",
					t->tid,states[t->state],tcyclePercent,tuserPercent,tkernelPercent);
		}
	}

	printf("\n");

	for(i = 0; i < count; i++)
		sll_destroy(procs[i].threads,true);
	free(procs);
	return EXIT_SUCCESS;
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

	if((dd = opendir("system:/processes")) >= 0) {
		while(readdir(&entry,dd)) {
			if(strcmp(entry.name,".") == 0 || strcmp(entry.name,"..") == 0)
				continue;

			/* build path */
			sprintf(ppath,"system:/processes/%s/info",entry.name);
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
		printe("Unable to open '%s'","system:/processes");
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
		"%*s%hu" "%*s%hu" "%*s%s" "%*s%u" "%*s%u" "%*s%u",
		&p->pid,&p->parentPid,&p->command,&p->textPages,&p->dataPages,&p->stackPages
	);
	p->threads = sll_create();

	/* read threads */
	sprintf(path,"system:/processes/%d/threads",pid);
	threads = open(path,IO_READ);
	if(threads < 0) {
		free(buf);
		sll_destroy(p->threads,true);
		return false;
	}
	while(readdir(&entry,threads)) {
		if(strcmp(entry.name,".") == 0 || strcmp(entry.name,"..") == 0)
			continue;

		/* build path */
		sprintf(ppath,"system:/processes/%d/threads/%s",pid,entry.name);
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
	char *buf = ps_readNode(fd);
	if(buf == NULL)
		return false;

	/* parse string; use separate u16 storage for state since we can't tell scanf that is a byte */
	u16 state;
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
