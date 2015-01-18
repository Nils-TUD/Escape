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

#include <dbg/cmd/step.h>
#include <dbg/console.h>
#include <task/proc.h>
#include <common.h>
#include <string.h>
#include <util.h>

int cons_cmd_step(OStream &os,size_t argc,char **argv) {
	if(Console::isHelp(argc,argv) || argc > 2) {
		os.writef("Usage: step [show]\n");
		return 0;
	}

	Thread *t = Thread::getRunning();
	IntrptStackFrame *kstack = t->getIntrptStack();
	if(argc == 2 && strcmp(argv[1],"show") == 0) {
		kstack->setFlags(kstack->getFlags() & ~(1 << 8));
		os.writef("Executing thread %d:%d:%s\n",t->getTid(),t->getProc()->getPid(),
		           t->getProc()->getProgram());
		Util::printStackTrace(os,Util::getUserStackTraceOf(t));
		Util::printUserState(os);
		return 0;
	}

	kstack->setFlags(kstack->getFlags() | (1 << 8));
	return CONS_EXIT;
}
