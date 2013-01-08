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
#include <sys/arch/i586/ioapic.h>
#include <sys/arch/i586/acpi.h>
#endif
#include <sys/task/proc.h>
#include <sys/task/sched.h>
#include <sys/task/signals.h>
#include <sys/task/thread.h>
#include <sys/task/lock.h>
#include <sys/task/event.h>
#include <sys/task/timer.h>
#include <sys/task/smp.h>
#include <sys/vfs/node.h>
#include <sys/vfs/vfs.h>
#include <sys/mem/cow.h>
#include <sys/mem/cache.h>
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/sharedmem.h>
#include <sys/mem/vmm.h>
#include <sys/mem/swapmap.h>
#include <sys/boot.h>
#include <sys/cpu.h>
#include <string.h>
#include <errno.h>

#define MAX_VIEWNAME_LEN	16

typedef void (*fView)(size_t argc,char **argv);
typedef struct {
	char name[MAX_VIEWNAME_LEN];
	fView func;
} sView;

static void view_printc(char c);
static void view_proc(size_t argc,char **argv);
static void view_thread(size_t argc,char **argv);
static void view_pdirall(size_t argc,char **argv);
static void view_pdiruser(size_t argc,char **argv);
static void view_pdirkernel(size_t argc,char **argv);
static void view_regions(size_t argc,char **argv);

static sProc *view_getProc(size_t argc,char **argv);
static const sThread *view_getThread(size_t argc,char **argv);

static sLines lines;
static sScreenBackup backup;
static sView views[] = {
	{"proc",(fView)view_proc},
	{"procs",(fView)proc_printAll},
	{"sched",(fView)sched_print},
	{"signals",(fView)sig_print},
	{"thread",(fView)view_thread},
	{"threads",(fView)thread_printAll},
	{"vfstree",(fView)vfs_node_printTree},
	{"gft",(fView)vfs_printGFT},
	{"msgs",(fView)vfs_printMsgs},
	{"cow",(fView)cow_print},
	{"cache",(fView)cache_print},
	{"kheap",(fView)kheap_print},
	{"pdirall",(fView)view_pdirall},
	{"pdiruser",(fView)view_pdiruser},
	{"pdirkernel",(fView)view_pdirkernel},
	{"regions",(fView)view_regions},
	{"pmem",(fView)pmem_print},
	{"pmemcont",(fView)pmem_printCont},
	{"pmemstack",(fView)pmem_printStack},
	{"shm",(fView)shm_print},
	{"swapmap",(fView)swmap_print},
	{"cpu",(fView)cpu_print},
#ifdef __i386__
	{"gdt",(fView)gdt_print},
	{"ioapic",(fView)ioapic_print},
	{"acpi",(fView)acpi_print},
#endif
	{"timer",(fView)timer_print},
	{"boot",(fView)boot_print},
	{"locks",(fView)lock_print},
	{"events",(fView)ev_print},
	{"smp",(fView)smp_print},
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
			views[i].func(argc,argv);
			lines_end(&lines);
			vid_unsetPrintFunc();
			break;
		}
	}
	if(i == ARRAY_SIZE(views))
		return -EINVAL;

	/* view the lines */
	vid_backup(backup.screen,&backup.row,&backup.col);
	cons_viewLines(&lines);
	lines_destroy(&lines);
	vid_restore(backup.screen,backup.row,backup.col);
	return 0;
}

static void view_printc(char c) {
	if(c == '\n')
		lines_newline(&lines);
	else
		lines_append(&lines,c);
}

static void view_proc(size_t argc,char **argv) {
	sProc *p = view_getProc(argc,argv);
	if(p != NULL)
		proc_print(p);
}
static void view_thread(size_t argc,char **argv) {
	const sThread *t = view_getThread(argc,argv);
	if(t != NULL)
		thread_print(t);
}
static void view_pdirall(size_t argc,char **argv) {
	sProc *p = view_getProc(argc,argv);
	if(p != NULL)
		paging_printPDir(&p->pagedir,PD_PART_ALL);
}
static void view_pdiruser(size_t argc,char **argv) {
	sProc *p = view_getProc(argc,argv);
	if(p != NULL)
		paging_printPDir(&p->pagedir,PD_PART_USER);
}
static void view_pdirkernel(size_t argc,char **argv) {
	sProc *p = view_getProc(argc,argv);
	if(p != NULL)
		paging_printPDir(&p->pagedir,PD_PART_KERNEL);
}
static void view_regions(size_t argc,char **argv) {
	if(argc < 3)
		proc_printAllRegions();
	else {
		sProc *p = view_getProc(argc,argv);
		if(p != NULL)
			vmm_print(p->pid);
	}
}

static sProc *view_getProc(size_t argc,char **argv) {
	sProc *p;
	if(argc > 2)
		p = proc_getByPid(atoi(argv[2]));
	else
		p = proc_getByPid(proc_getRunning());
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
