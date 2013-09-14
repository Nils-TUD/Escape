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
#include <sys/vfs/openfile.h>
#include <sys/mem/copyonwrite.h>
#include <sys/mem/cache.h>
#include <sys/mem/kheap.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/physmem.h>
#include <sys/mem/physmemareas.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/swapmap.h>
#include <sys/boot.h>
#include <sys/cpu.h>
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
	{"signals",		Signals::print},
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
	{"cpu",			CPU::print},
#ifdef __i386__
	{"gdt",			GDT::print},
	{"ioapic",		IOAPIC::print},
	{"acpi",		ACPI::print},
#endif
	{"timer",		Timer::print},
	{"boot",		Boot::print},
	{"locks",		Lock::print},
	{"events",		Event::print},
	{"smp",			SMP::print},
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
	if(p != NULL)
		p->print(os);
}
static void view_thread(OStream &os) {
	const Thread *t = view_getThread(os);
	if(t != NULL)
		t->print(os);
}
static void view_pdirall(OStream &os) {
	Proc *p = view_getProc(os);
	if(p != NULL)
		p->getPageDir()->print(os,PD_PART_ALL);
}
static void view_pdiruser(OStream &os) {
	Proc *p = view_getProc(os);
	if(p != NULL)
		p->getPageDir()->print(os,PD_PART_USER);
}
static void view_pdirkernel(OStream &os) {
	Proc *p = view_getProc(os);
	if(p != NULL)
		p->getPageDir()->print(os,PD_PART_KERNEL);
}
static void view_regions(OStream &os) {
	if(_argc < 3)
		Proc::printAllRegions(os);
	else {
		Proc *p = view_getProc(os);
		if(p != NULL)
			p->getVM()->printRegions(os);
	}
}

static Proc *view_getProc(OStream &os) {
	Proc *p;
	if(_argc > 2)
		p = Proc::getByPid(atoi(_argv[2]));
	else
		p = Proc::getByPid(Proc::getRunning());
	if(p == NULL)
		os.writef("Unable to find process '%s'\n",_argv[2]);
	return p;
}

static const Thread *view_getThread(OStream &os) {
	const Thread *t;
	if(_argc > 2)
		t = Thread::getById(atoi(_argv[2]));
	else
		t = Thread::getRunning();
	if(t == NULL)
		os.writef("Unable to find thread '%s'\n",_argv[2]);
	return t;
}
