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
#include <sys/mem/copyonwrite.h>
#include <sys/mem/cache.h>
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/mem/physmem.h>
#include <sys/mem/physmemareas.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/swapmap.h>
#include <sys/boot.h>
#include <sys/cpu.h>
#include <string.h>
#include <errno.h>

#define MAX_VIEWNAME_LEN	16

typedef void (*view_func)(size_t argc,char **argv);
struct View {
	char name[MAX_VIEWNAME_LEN];
	view_func func;
};

static void view_printc(char c);
static void view_proc(size_t argc,char **argv);
static void view_thread(size_t argc,char **argv);
static void view_pdirall(size_t argc,char **argv);
static void view_pdiruser(size_t argc,char **argv);
static void view_pdirkernel(size_t argc,char **argv);
static void view_regions(size_t argc,char **argv);

static Proc *view_getProc(size_t argc,char **argv);
static const Thread *view_getThread(size_t argc,char **argv);

static Lines *linesptr;
static ScreenBackup backup;
static View views[] = {
	{"proc",(view_func)view_proc},
	{"procs",(view_func)Proc::printAll},
	{"sched",(view_func)Sched::print},
	{"signals",(view_func)Signals::print},
	{"thread",(view_func)view_thread},
	{"threads",(view_func)Thread::printAll},
	{"vfstree",(view_func)vfs_node_printTree},
	{"gft",(view_func)vfs_printGFT},
	{"msgs",(view_func)vfs_printMsgs},
	{"cow",(view_func)CopyOnWrite::print},
	{"cache",(view_func)Cache::print},
	{"kheap",(view_func)KHeap::print},
	{"pdirall",(view_func)view_pdirall},
	{"pdiruser",(view_func)view_pdiruser},
	{"pdirkernel",(view_func)view_pdirkernel},
	{"regions",(view_func)view_regions},
	{"pmem",(view_func)PhysMem::print},
	{"pmemcont",(view_func)PhysMem::printCont},
	{"pmemstack",(view_func)PhysMem::printStack},
	{"pmemareas",(view_func)PhysMemAreas::print},
	{"swapmap",(view_func)SwapMap::print},
	{"cpu",(view_func)CPU::print},
#ifdef __i386__
	{"gdt",(view_func)gdt_print},
	{"ioapic",(view_func)ioapic_print},
	{"acpi",(view_func)acpi_print},
#endif
	{"timer",(view_func)Timer::print},
	{"boot",(view_func)boot_print},
	{"locks",(view_func)Lock::print},
	{"events",(view_func)Event::print},
	{"smp",(view_func)SMP::print},
};

int cons_cmd_view(size_t argc,char **argv) {
	size_t i;
	if(Console::isHelp(argc,argv) || argc < 2) {
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

	Lines lines;
	linesptr = &lines;
	for(i = 0; i < ARRAY_SIZE(views); i++) {
		if(strcmp(views[i].name,argv[1]) == 0) {
			/* redirect prints */
			vid_setPrintFunc(view_printc);
			views[i].func(argc,argv);
			lines.endLine();
			vid_unsetPrintFunc();
			break;
		}
	}
	if(i == ARRAY_SIZE(views))
		return -EINVAL;

	/* view the lines */
	vid_backup(backup.screen,&backup.row,&backup.col);
	Console::viewLines(&lines);
	vid_restore(backup.screen,backup.row,backup.col);
	return 0;
}

static void view_printc(char c) {
	if(c == '\n')
		linesptr->newLine();
	else
		linesptr->append(c);
}

static void view_proc(size_t argc,char **argv) {
	Proc *p = view_getProc(argc,argv);
	if(p != NULL)
		p->print();
}
static void view_thread(size_t argc,char **argv) {
	const Thread *t = view_getThread(argc,argv);
	if(t != NULL)
		t->print();
}
static void view_pdirall(size_t argc,char **argv) {
	Proc *p = view_getProc(argc,argv);
	if(p != NULL)
		paging_printPDir(p->getPageDir(),PD_PART_ALL);
}
static void view_pdiruser(size_t argc,char **argv) {
	Proc *p = view_getProc(argc,argv);
	if(p != NULL)
		paging_printPDir(p->getPageDir(),PD_PART_USER);
}
static void view_pdirkernel(size_t argc,char **argv) {
	Proc *p = view_getProc(argc,argv);
	if(p != NULL)
		paging_printPDir(p->getPageDir(),PD_PART_KERNEL);
}
static void view_regions(size_t argc,char **argv) {
	if(argc < 3)
		Proc::printAllRegions();
	else {
		Proc *p = view_getProc(argc,argv);
		if(p != NULL)
			p->getVM()->printRegions();
	}
}

static Proc *view_getProc(size_t argc,char **argv) {
	Proc *p;
	if(argc > 2)
		p = Proc::getByPid(atoi(argv[2]));
	else
		p = Proc::getByPid(Proc::getRunning());
	if(p == NULL)
		vid_printf("Unable to find process '%s'\n",argv[2]);
	return p;
}

static const Thread *view_getThread(size_t argc,char **argv) {
	const Thread *t;
	if(argc > 2)
		t = Thread::getById(atoi(argv[2]));
	else
		t = Thread::getRunning();
	if(t == NULL)
		vid_printf("Unable to find thread '%s'\n",argv[2]);
	return t;
}
