/**
 * $Id$
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

#include <sys/common.h>
#include <sys/dbg/console.h>
#include <sys/dbg/cmd/view.h>
#ifdef __i386__
#include <sys/arch/i586/gdt.h>
#endif
#include <sys/task/proc.h>
#include <sys/task/sched.h>
#include <sys/task/signals.h>
#include <sys/task/thread.h>
#include <sys/task/lock.h>
#include <sys/task/event.h>
#include <sys/vfs/node.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/request.h>
#include <sys/mem/cow.h>
#include <sys/mem/cache.h>
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/sharedmem.h>
#include <sys/mem/vmm.h>
#include <sys/task/timer.h>
#include <sys/boot.h>
#include <sys/cpu.h>
#include <string.h>
#include <errors.h>

#define MAX_VIEWNAME_LEN	16

typedef void (*fView)(size_t argc,char **argv);
typedef struct {
	char name[MAX_VIEWNAME_LEN];
	fView func;
} sView;

static void view_printc(char c);
static void view_proc(size_t argc,char **argv);
static void view_procs(void);
static void view_sched(void);
static void view_signals(void);
static void view_thread(size_t argc,char **argv);
static void view_threads(void);
static void view_vfstree(void);
static void view_gft(void);
static void view_msgs(void);
static void view_cow(void);
static void view_cache(void);
static void view_kheap(void);
static void view_pdirall(size_t argc,char **argv);
static void view_pdiruser(size_t argc,char **argv);
static void view_pdirkernel(size_t argc,char **argv);
static void view_regions(size_t argc,char **argv);
static void view_pmem(void);
static void view_pmemcont(void);
static void view_pmemstack(void);
static void view_shm(void);
static void view_cpu(void);
#ifdef __i386__
static void view_gdt(void);
#endif
static void view_timer(void);
static void view_boot(void);
static void view_requests(void);
static void view_locks(void);
static void view_events(void);

static const sProc *view_getProc(size_t argc,char **argv);
static const sThread *view_getThread(size_t argc,char **argv);

static int err = 0;
static sLines lines;
static sScreenBackup backup;
static sView views[] = {
	{"proc",(fView)view_proc},
	{"procs",(fView)view_procs},
	{"sched",(fView)view_sched},
	{"signals",(fView)view_signals},
	{"thread",(fView)view_thread},
	{"threads",(fView)view_threads},
	{"vfstree",(fView)view_vfstree},
	{"gft",(fView)view_gft},
	{"msgs",(fView)view_msgs},
	{"cow",(fView)view_cow},
	{"cache",(fView)view_cache},
	{"kheap",(fView)view_kheap},
	{"pdirall",(fView)view_pdirall},
	{"pdiruser",(fView)view_pdiruser},
	{"pdirkernel",(fView)view_pdirkernel},
	{"regions",(fView)view_regions},
	{"pmem",(fView)view_pmem},
	{"pmemcont",(fView)view_pmemcont},
	{"pmemstack",(fView)view_pmemstack},
	{"shm",(fView)view_shm},
	{"cpu",(fView)view_cpu},
#ifdef __i386__
	{"gdt",(fView)view_gdt},
#endif
	{"timer",(fView)view_timer},
	{"boot",(fView)view_boot},
	{"requests",(fView)view_requests},
	{"locks",(fView)view_locks},
	{"events",(fView)view_events},
};

int cons_cmd_view(size_t argc,char **argv) {
	size_t i;
	int res;
	if(argc < 2) {
		vid_printf("Usage: %s <what>\n",argv[0]);
		vid_printf("Available 'whats':\n");
		for(i = 0; i < ARRAY_SIZE(views); i++) {
			if(i % 6 == 0) {
				if(i > 0)
					vid_printf("\n");
				vid_printf("\t");
			}
			vid_printf("%s ",views[i].name);
		}
		vid_printf("\n");
		return 0;
	}

	for(i = 0; i < ARRAY_SIZE(views); i++) {
		if(strcmp(views[i].name,argv[1]) == 0) {
			/* create lines and redirect prints */
			if((res = lines_create(&lines)) < 0)
				return res;
			vid_setPrintFunc(view_printc);
			err = 0;
			views[i].func(argc,argv);
			if(err < 0) {
				vid_unsetPrintFunc();
				lines_destroy(&lines);
				return err;
			}
			lines_end(&lines);
			vid_unsetPrintFunc();
			break;
		}
	}
	if(i == ARRAY_SIZE(views))
		return ERR_INVALID_ARGS;

	/* view the lines */
	vid_backup(backup.screen,&backup.row,&backup.col);
	cons_viewLines(&lines);
	lines_destroy(&lines);
	vid_restore(backup.screen,backup.row,backup.col);
	return 0;
}

static void view_printc(char c) {
	if(err < 0)
		return;
	if(c == '\n')
		err = lines_newline(&lines);
	else
		lines_append(&lines,c);
}

static void view_proc(size_t argc,char **argv) {
	const sProc *p = view_getProc(argc,argv);
	if(p != NULL)
		proc_print(p);
}
static void view_procs(void) {
	proc_printAll();
}
static void view_sched(void) {
	sched_print();
}
static void view_signals(void) {
	sig_print();
}
static void view_thread(size_t argc,char **argv) {
	const sThread *t = view_getThread(argc,argv);
	if(t != NULL)
		thread_print(t);
}
static void view_threads(void) {
	thread_printAll();
}
static void view_vfstree(void) {
	vfs_node_printTree();
}
static void view_gft(void) {
	vfs_printGFT();
}
static void view_msgs(void) {
	vfs_printMsgs();
}
static void view_cow(void) {
	cow_print();
}
static void view_cache(void) {
	cache_print();
}
static void view_kheap(void) {
	kheap_print();
}
static void view_pdirall(size_t argc,char **argv) {
	const sProc *p = view_getProc(argc,argv);
	if(p != NULL)
		paging_printPDir(&p->pagedir,PD_PART_ALL);
}
static void view_pdiruser(size_t argc,char **argv) {
	const sProc *p = view_getProc(argc,argv);
	if(p != NULL)
		paging_printPDir(&p->pagedir,PD_PART_USER);
}
static void view_pdirkernel(size_t argc,char **argv) {
	const sProc *p = view_getProc(argc,argv);
	if(p != NULL)
		paging_printPDir(&p->pagedir,PD_PART_KERNEL);
}
static void view_regions(size_t argc,char **argv) {
	if(argc < 3)
		proc_printAllRegions();
	else {
		const sProc *p = view_getProc(argc,argv);
		if(p != NULL)
			vmm_print(p);
	}
}
static void view_pmem(void) {
	pmem_print(MM_DEF | MM_CONT);
}
static void view_pmemcont(void) {
	pmem_print(MM_CONT);
}
static void view_pmemstack(void) {
	pmem_print(MM_DEF);
}
static void view_shm(void) {
	shm_print();
}
static void view_cpu(void) {
	cpu_print();
}
#ifdef __i386__
static void view_gdt(void) {
	gdt_print();
}
#endif
static void view_timer(void) {
	timer_print();
}
static void view_boot(void) {
	boot_print();
}
static void view_requests(void) {
	vfs_req_printAll();
}
static void view_locks(void) {
	lock_print();
}
static void view_events(void) {
	ev_print();
}

static const sProc *view_getProc(size_t argc,char **argv) {
	const sProc *p;
	if(argc > 2)
		p = proc_getByPid(atoi(argv[2]));
	else
		p = proc_getRunning();
	if(p == NULL)
		vid_printf("Unable to find process '%s'\n",argv[2]);
	return p;
}

static const sThread *view_getThread(size_t argc,char **argv) {
	const sThread *t;
	if(argc > 2)
		t = thread_getById(atoi(argv[2]));
	else
		t = thread_getRunning();
	if(t == NULL)
		vid_printf("Unable to find thread '%s'\n",argv[2]);
	return t;
}
