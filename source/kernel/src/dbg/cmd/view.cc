/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <common.h>
#include <dbg/console.h>
#include <dbg/cmd/view.h>
#if defined(__x86__)
#	include <arch/x86/gdt.h>
#	include <arch/x86/ioapic.h>
#	include <arch/x86/acpi.h>
#endif
#include <task/proc.h>
#include <task/sched.h>
#include <task/signals.h>
#include <task/thread.h>
#include <task/timer.h>
#include <task/smp.h>
#include <vfs/node.h>
#include <vfs/vfs.h>
#include <vfs/openfile.h>
#include <mem/copyonwrite.h>
#include <mem/cache.h>
#include <mem/kheap.h>
#include <mem/pagedir.h>
#include <mem/physmem.h>
#include <mem/physmemareas.h>
#include <mem/virtmem.h>
#include <mem/swapmap.h>
#include <interrupts.h>
#include <boot.h>
#include <cpu.h>
#include <string.h>
#include <errno.h>

#define MAX_VIEWNAME_LEN	16

class ViewOStream : public OStream {
public:
	explicit ViewOStream(Lines *l) : OStream(), lines(l) {
	}

private:
	virtual void writec(char c) {
		if(c == '\n')
			lines->newLine();
		else if(c != '\0')
			lines->append(c);
	}

	Lines *lines;
};

typedef void (*view_func)(OStream &os);
struct View {
	char name[MAX_VIEWNAME_LEN];
	view_func func;
};

static void view_proc(OStream &os);
static void view_thread(OStream &os);
static void view_pdirall(OStream &os);
static void view_pdiruser(OStream &os);
static void view_pdirkernel(OStream &os);
static void view_regions(OStream &os);

static Proc *view_getProc(OStream &os);
static const Thread *view_getThread(OStream &os);

static View views[] = {
	{"proc",		view_proc},
	{"procs",		Proc::printAll},
	{"sched",		Sched::print},
	{"thread",		view_thread},
	{"threads",		Thread::printAll},
	{"vfstree",		VFSNode::printTree},
	{"gft",			OpenFile::printAll},
	{"msgs",		VFS::printMsgs},
	{"cow",			CopyOnWrite::print},
	{"cache",		Cache::print},
	{"kheap",		KHeap::print},
	{"pdirall",		view_pdirall},
	{"pdiruser",	view_pdiruser},
	{"pdirkernel",	view_pdirkernel},
	{"regions",		view_regions},
	{"pmem",		PhysMem::print},
	{"pmemcont",	PhysMem::printCont},
	{"pmemstack",	PhysMem::printStack},
	{"pmemareas",	PhysMemAreas::print},
	{"swapmap",		SwapMap::print},
#if defined(__i586__)
	{"gdt",			GDT::print},
#endif
#if defined(__x86__)
	{"ioapic",		IOAPIC::print},
	{"acpi",		ACPI::print},
#endif
	{"timer",		Timer::print},
	{"boot",		Boot::print},
	{"events",		Sched::printEventLists},
	{"smp",			SMP::print},
	{"mounts",		MountSpace::printAll},
};

static size_t _argc;
static char **_argv;

int cons_cmd_view(OStream &os,size_t argc,char **argv) {
	if(Console::isHelp(argc,argv) || argc < 2) {
		os.writef("Usage: %s <what>\n",argv[0]);
		os.writef("Available 'whats':\n");
		for(size_t i = 0; i < ARRAY_SIZE(views); i++) {
			if(i % 6 == 0) {
				if(i > 0)
					os.writef("\n");
				os.writef("\t");
			}
			os.writef("%s ",views[i].name);
		}
		os.writef("\n");
		return 0;
	}

	Lines lines;
	ViewOStream vos(&lines);
	_argc = argc;
	_argv = argv;
	size_t i;
	for(i = 0; i < ARRAY_SIZE(views); i++) {
		if(strcmp(views[i].name,argv[1]) == 0) {
			views[i].func(vos);
			lines.endLine();
			break;
		}
	}
	if(i == ARRAY_SIZE(views))
		return -EINVAL;

	/* view the lines */
	Console::viewLines(os,&lines);
	return 0;
}

static void view_proc(OStream &os) {
	Proc *p = view_getProc(os);
	if(p != NULL) {
		p->print(os);
		Proc::relRef(p);
	}
}
static void view_thread(OStream &os) {
	const Thread *t = view_getThread(os);
	if(t != NULL) {
		t->print(os);
		Thread::relRef(t);
	}
}
static void view_pdirall(OStream &os) {
	Proc *p = view_getProc(os);
	if(p != NULL) {
		p->getPageDir()->print(os,PD_PART_ALL);
		Proc::relRef(p);
	}
}
static void view_pdiruser(OStream &os) {
	Proc *p = view_getProc(os);
	if(p != NULL) {
		p->getPageDir()->print(os,PD_PART_USER);
		Proc::relRef(p);
	}
}
static void view_pdirkernel(OStream &os) {
	Proc *p = view_getProc(os);
	if(p != NULL) {
		p->getPageDir()->print(os,PD_PART_KERNEL);
		Proc::relRef(p);
	}
}
static void view_regions(OStream &os) {
	if(_argc < 3)
		Proc::printAllRegions(os);
	else {
		Proc *p = view_getProc(os);
		if(p != NULL) {
			p->getVM()->printRegions(os);
			Proc::relRef(p);
		}
	}
}

static Proc *view_getProc(OStream &os) {
	Proc *p;
	if(_argc > 2)
		p = Proc::getRef(atoi(_argv[2]));
	else
		p = Proc::getRef(Proc::getRunning());
	if(p == NULL)
		os.writef("Unable to find process '%s'\n",_argv[2]);
	return p;
}

static const Thread *view_getThread(OStream &os) {
	const Thread *t;
	if(_argc > 2)
		t = Thread::getRef(atoi(_argv[2]));
	else
		t = Thread::getRef(Thread::getRunning()->getTid());
	if(t == NULL)
		os.writef("Unable to find thread '%s'\n",_argv[2]);
	return t;
}
