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
#include <esc/mem/heap.h>
#include <esc/io/console.h>
#include <esc/io/file.h>
#include <esc/io/ifilestream.h>
#include <esc/exceptions/io.h>
#include <esc/exceptions/cmdargs.h>
#include <esc/util/vector.h>
#include <esc/util/cmdargs.h>
#include <width.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
	u32 pages;
	u32 ownFrames;
	u32 sharedFrames;
	u32 swapped;
	uLongLong ucycleCount;
	uLongLong kcycleCount;
	sVector *threads;
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
static sVector *ps_getProcs(void);
static sProcess *ps_readProc(const char *pid);

static void usage(const char *name) {
	u32 i;
	cerr->writef(cerr,"Usage: %s [-t][-n <count>][-s <sort>]\n",name);
	cerr->writef(cerr,"	-t			: Print threads, too\n");
	cerr->writef(cerr,"	-n <count>	: Print first <count> processes\n");
	cerr->writef(cerr,"	-s <sort>	: Sort by ");
	for(i = 0; i < ARRAY_SIZE(sorts); i++) {
		cerr->writef(cerr,"'%s'",sorts[i].name);
		if(i < ARRAY_SIZE(sorts) - 1)
			cerr->writef(cerr,", ");
	}
	cerr->writef(cerr,"\n");
	exit(EXIT_FAILURE);
}

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
	"Ill ",
	"Run ",
	"Rdy ",
	"Blk ",
	"Zom "
};
static u8 sort = SORT_PID;

int main(int argc,const char *argv[]) {
	sVector *procs;
	sProcess *p;
	sPThread *t;
	u64 totalCycles;
	u32 i,numProcs = 0;
	bool printThreads = false;
	char *ssort = (char*)"pid";
	sCmdArgs *args;

	TRY {
		args = cmdargs_create(argc,argv,CA_NO_FREE);
		args->parse(args,"s=s n=d t",&ssort,&numProcs,&printThreads);
		if(args->isHelp)
			usage(argv[0]);
	}
	CATCH(CmdArgsException,e) {
		cerr->writef(cerr,"Invalid arguments: %s\n",e->toString(e));
		usage(argv[0]);
	}
	ENDCATCH

	for(i = 0; i < ARRAY_SIZE(sorts); i++) {
		if(strcmp(ssort,sorts[i].name) == 0) {
			sort = sorts[i].type;
			break;
		}
	}

	procs = ps_getProcs();

	/* sum total cycles and assign cycles to processes */
	totalCycles = 0;
	{
		vforeach(procs,p) {
			u64 procUCycles = 0,procKCycles = 0;
			vforeach(p->threads,t) {
				totalCycles += t->ucycleCount.val64 + t->kcycleCount.val64;
				procUCycles += t->ucycleCount.val64;
				procKCycles += t->kcycleCount.val64;
			}
			p->ucycleCount.val64 = procUCycles;
			p->kcycleCount.val64 = procKCycles;
		}
	}

	/* sort */
	vec_sortCustom(procs,compareProcs);

	if(numProcs == 0)
		numProcs = procs->count;

	/* determine max-values (we want to have a min-width here :)) */
	u32 maxPid = 100;
	u32 maxPpid = 100;
	u32 maxPmem = 100;
	u32 maxVmem = 100;
	u32 maxSmem = 100;
	u32 maxShmem = 100;
	{
		vforeach(procs,p) {
			if(p->pid > maxPid)
				maxPid = p->pid;
			if(p->parentPid > maxPpid)
				maxPpid = p->parentPid;
			if(p->ownFrames > maxPmem)
				maxPmem = p->ownFrames;
			if(p->sharedFrames > maxShmem)
				maxShmem = p->sharedFrames;
			if(p->swapped > maxSmem)
				maxSmem = p->swapped;
			if(p->pages > maxVmem)
				maxVmem = p->pages;
			if(printThreads) {
				vforeach(p->threads,t) {
					if(t->tid > maxPpid)
						maxPpid = t->tid;
				}
			}
		}
	}
	maxPid = getuwidth(maxPid,10);
	maxPpid = getuwidth(maxPpid,10);
	/* display in KiB, its in pages, i.e. 4 KiB blocks */
	maxPmem = getuwidth(maxPmem * 4,10);
	maxVmem = getuwidth(maxVmem * 4,10);
	maxSmem = getuwidth(maxSmem * 4,10);
	maxShmem = getuwidth(maxShmem * 4,10);

	/* now print processes */
	cout->writef(cout,
		"%*sPID%*sPPID%*sPMEM%*sSHMEM%*sVMEM%*sSMEM STATE  %%CPU (USER,KERNEL) COMMAND\n",
		maxPid - 3,"",maxPpid - 1,"",maxPmem - 2,"",maxShmem - 2,"",maxVmem - 2,"",maxSmem - 2,""
	);

	sIterator it = vec_iteratorIn(procs,0,numProcs);
	while(it.hasNext(&it)) {
		u64 procCycles;
		u32 userPercent,kernelPercent;
		float cyclePercent;
		p = (sProcess*)it.next(&it);

		procCycles = p->ucycleCount.val64 + p->kcycleCount.val64;
		cyclePercent = (float)(100. / (totalCycles / (double)procCycles));
		userPercent = (u32)(100. / (procCycles / (double)p->ucycleCount.val64));
		kernelPercent = (u32)(100. / (procCycles / (double)p->kcycleCount.val64));
		cout->writef(cout,"%*u   %*u %*uK  %*uK %*uK %*uK -     %4.1f%% (%3d%%,%3d%%)   %s\n",
				maxPid,p->pid,maxPpid,p->parentPid,
				maxPmem,p->ownFrames * 4,
				maxShmem,p->sharedFrames * 4,
				maxVmem,p->pages * 4,
				maxSmem,p->swapped * 4,
				cyclePercent,userPercent,kernelPercent,p->command);

		if(printThreads) {
			sIterator tit = vec_iterator(p->threads);
			while(tit.hasNext(&tit)) {
				t = (sPThread*)tit.next(&tit);
				u64 threadCycles = t->ucycleCount.val64 + t->kcycleCount.val64;
				float tcyclePercent = (float)(100. / (totalCycles / (double)threadCycles));
				u32 tuserPercent = (u32)(100. / (threadCycles / (double)t->ucycleCount.val64));
				u32 tkernelPercent = (u32)(100. / (threadCycles / (double)t->kcycleCount.val64));
				cout->writef(cout,"  %c\xC4%*s%*d%*s%s  %4.1f%% (%3d%%,%3d%%)\n",
						tit.hasNext(&tit) ? 0xC3 : 0xC0,
						maxPid - 3,"",maxPpid,t->tid,12 + maxPmem + maxShmem + maxVmem + maxSmem,"",
						states[t->state],tcyclePercent,tuserPercent,tkernelPercent);
			}
		}
	}

	cout->writec(cout,'\n');
	vforeach(procs,p)
		vec_destroy(p->threads,true);
	vec_destroy(procs,true);
	return EXIT_SUCCESS;
}

static int compareProcs(const void *a,const void *b) {
	sProcess *p1 = *(sProcess**)a;
	sProcess *p2 = *(sProcess**)b;
	switch(sort) {
		case SORT_PID:
			/* ascending */
			return p1->pid - p2->pid;
		case SORT_PPID:
			/* ascending */
			return p1->parentPid - p2->parentPid;
		case SORT_MEM:
			/* descending */
			return p2->pages - p1->pages;
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

static sVector *ps_getProcs(void) {
	sVector *v = vec_create(sizeof(sProcess*));
	sDirEntry *f;
	sFile *d = file_get("/system/processes");
	sVector *files = d->listFiles(d,false);
	vforeach(files,f) {
		TRY {
			sProcess *p = ps_readProc(f->name);
			vec_add(v,&p);
		}
		CATCH(IOException,e) {
			cerr->writef(cerr,"Unable to read process with pid %s\n",f->name);
		}
		ENDCATCH
	}
	vec_destroy(files,true);
	d->destroy(d);
	return v;
}

static sProcess *ps_readProc(const char *pid) {
	char path[MAX_PATH_LEN + 1];
	char tpath[MAX_PATH_LEN + 1];
	sDirEntry *e;
	sFile *d;
	sIStream *s;
	sVector *files;
	sProcess *p = (sProcess*)heap_alloc(sizeof(sProcess));

	/* read process info */
	snprintf(path,sizeof(path),"/system/processes/%s/info",pid);
	s = ifstream_open(path,IO_READ);
	s->readf(s,
		"%*s%hu" "%*s%hu" "%*s%s" "%*s%u" "%*s%u" "%*s%u" "%*s%u",
		&p->pid,&p->parentPid,&p->command,&p->pages,&p->ownFrames,&p->sharedFrames,&p->swapped
	);
	s->close(s);

	/* read threads */
	p->threads = vec_create(sizeof(sPThread*));
	snprintf(path,sizeof(path),"/system/processes/%s/threads",pid);
	d = file_get(path);
	files = d->listFiles(d,false);
	vforeach(files,e) {
		u16 state;
		sPThread *t = heap_alloc(sizeof(sPThread));
		snprintf(tpath,sizeof(tpath),"/system/processes/%s/threads/%s",pid,e->name);
		s = ifstream_open(tpath,IO_READ);
		/* parse string; use separate u16 storage for state since we can't tell scanf that is a byte */
		s->readf(s,
			"%*s%hu" "%*s%hu" "%*s%hu" "%*s%u" "%*s%*u" "%*s%8x%8x" "%*s%8x%8x",
			&t->tid,&t->pid,&state,&t->stackPages,
			&t->ucycleCount.val32.upper,&t->ucycleCount.val32.lower,
			&t->kcycleCount.val32.upper,&t->kcycleCount.val32.lower
		);
		t->state = state;
		s->close(s);
		vec_add(p->threads,&t);
	}
	vec_destroy(files,true);
	return p;
}
