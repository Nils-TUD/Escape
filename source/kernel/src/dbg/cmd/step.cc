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
#include <sys/dbg/cmd/step.h>
#include <sys/task/proc.h>
#include <sys/util.h>
#include <string.h>

int cons_cmd_step(size_t argc,char **argv) {
	if(cons_isHelp(argc,argv) || argc > 2) {
		vid_printf("Usage: step [show]\n");
		return 0;
	}

#ifdef __i386__
	Thread *t = Thread::getRunning();
	sIntrptStackFrame *kstack = t->getIntrptStack();
	if(argc == 2 && strcmp(argv[1],"show") == 0) {
		kstack->eflags &= ~(1 << 8);
		vid_printf("Executing thread %d:%d:%s\n",t->tid,t->proc->getPid(),t->proc->getCommand());
		util_printStackTrace(util_getUserStackTraceOf(t));
		util_printUserState();
		return 0;
	}

	kstack->eflags |= 1 << 8;
	return CONS_EXIT;
#else
	vid_printf("Sorry, not supported\n");
	return 0;
#endif
}
