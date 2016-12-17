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

#include <esc/proto/vterm.h>
#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <esc/env.h>
#include <esc/file.h>
#include <info/process.h>
#include <info/thread.h>
#include <sys/arch.h>
#include <sys/common.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <sys/messages.h>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace esc;
using namespace std;
using namespace info;

struct sort {
	const static int TID	= 0;
	const static int NAME	= 2;
	const static int STACK	= 3;
	const static int STATE	= 4;
	const static int PRIO	= 5;
	const static int CPU	= 6;
	const static int TIME	= 7;

	int type;
	string name;
};

static bool compareThreads(const thread* a,const thread* b);

static struct sort sorts[] = {
	{sort::TID,"tid"},
	{sort::NAME,"proc"},
	{sort::STACK,"stack"},
	{sort::STATE,"state"},
	{sort::PRIO,"prio"},
	{sort::CPU,"cpu"},
	{sort::TIME,"time"},
};
static const char *states[] = {
	"RUN",
	"RDY",
	"BLK",
	"ZOM",
	"SUS",
	"SUS",
	"SUS",
};
static int sortcol = sort::TID;

static void usage(const char *name) {
	size_t i;
	serr << "Usage: " << name <<" [-s <sort>]" << '\n';
	serr << "    <sort> can be ";
	for(i = 0; i < ARRAY_SIZE(sorts); i++) {
		serr << "'" << sorts[i].name << "'";
		if(i < ARRAY_SIZE(sorts) - 1)
			serr << ", ";
	}
	serr << '\n';
	serr << '\n';
	serr << "Explanation of the displayed information:\n";
	serr << "ID    : the thread id\n";
	serr << "STATE : thread state (RUN,RDY,BLK,ZOM,SUS)\n";
	serr << "PRIO  : the thread priority (high means high priority)\n";
	serr << "CPU   : the CPU on which the thread is currently running on\n";
	serr << "STACK : the amount of physical memory used for the stack\n";
	serr << "SCHED : the number of times the thread has been scheduled\n";
	serr << "SYSC  : the number of system-calls the thread has executed\n";
	serr << "TIME  : the CPU time used by the thread so far in minutes:seconds.milliseconds\n";
	serr << "CPU%  : the CPU usage in the last second in percent\n";
	serr << "PROC  : the name of the process the thread belongs to\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	string ssort("tid");

	// parse args
	cmdargs args(argc,argv,cmdargs::NO_FREE);
	try {
		args.parse("s=s",&ssort);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		errmsg("Invalid arguments: " << e.what());
		usage(argv[0]);
	}

	// determine sort
	for(size_t i = 0; i < ARRAY_SIZE(sorts); i++) {
		if(ssort == sorts[i].name) {
			sortcol = sorts[i].type;
			break;
		}
	}

	vector<thread*> threads = thread::get_list();

	// determine max-values (we want to have a min-width here :))
	thread::tid_type maxTid = 10;
	thread::size_type maxStack = 10000;
	thread::size_type maxScheds = 10000;
	thread::size_type maxSyscalls = 1000;
	thread::time_type maxRuntime = 100;
	thread::cycle_type totalCycles = 0;
	for(auto it = threads.begin(); it != threads.end(); ++it) {
		thread *t = *it;
		if(t->tid() > maxTid)
			maxTid = t->tid();
		if(t->stackPages() > maxStack)
			maxStack = t->stackPages();
		if(t->schedCount() > maxScheds)
			maxScheds = t->schedCount();
		if(t->syscalls() > maxSyscalls)
			maxSyscalls = t->syscalls();
		if(t->runtime() > maxRuntime)
			maxRuntime = t->runtime();
		totalCycles += t->cycles();
	}
	maxTid = count_digits(maxTid,10);
	// display in KiB, its in pages
	maxStack = count_digits((maxStack * PAGE_SIZE) / 1024,10);
	maxScheds = count_digits(maxScheds,10);
	maxSyscalls = count_digits(maxSyscalls,10);
	maxRuntime = count_digits(maxRuntime / (1000000 * 60),10);

	// sort
	std::sort(threads.begin(),threads.end(),compareThreads);

	// get console-size
	VTerm vterm(env::get("TERM").c_str());
	Screen::Mode mode = vterm.getMode();

	// print header
	sout << fmt("ID",maxTid);
	sout << " " << "STATE";
	sout << " " << "PRIO";
	sout << " " << " CPU";
	sout << " " << fmt("STACK",maxStack);
	sout << " " << fmt("SCHED",maxScheds);
	sout << " " << fmt("SYSC",maxSyscalls);
	sout << " " << fmt("TIME",maxRuntime + 7);
	sout << "   CPU% PROC" << '\n';

	// calc with to the proc-command
	size_t width2cmd = maxTid + SSTRLEN("STATE") + SSTRLEN("PRIO") + SSTRLEN(" CPU") + maxStack +
			maxScheds + maxSyscalls + maxRuntime + 7;
	width2cmd += 8 + SSTRLEN("   CPU ");

	// print threads
	for(auto it = threads.begin(); it != threads.end(); ++it) {
		thread *t = *it;
		float cyclePercent = 0;
		if(t->cycles() != 0)
			cyclePercent = (float)(100. / (totalCycles / (double)t->cycles()));
		size_t cmdwidth = min(mode.cols - (width2cmd + 1 + count_digits(t->pid(),10)),
				t->procName().length());
		string procName = t->procName().substr(0,cmdwidth);

		sout << fmt(t->tid(),maxTid) << " ";
		sout << fmt(states[t->state()],5) << " ";
		sout << fmt(t->prio(),4) << " ";
		if(t->state() == 0)
			sout << fmt(t->cpu(),4);
		else
			sout << fmt("-",4);
		sout << " ";
		sout << fmt((t->stackPages() * PAGE_SIZE) / 1024,maxStack - 1) << "K ";
		sout << fmt(t->schedCount(),maxScheds) << " ";
		sout << fmt(t->syscalls(),maxSyscalls) << " ";
		thread::time_type time = t->runtime() / 1000;
		sout << fmt(time / (1000 * 60),maxRuntime) << ":";
		time %= 1000 * 60;
		sout << fmt(time / 1000,"-0",2) << ".";
		time %= 1000;
		sout << fmt(time,"0",3) << " ";
		sout << fmt(cyclePercent,5, 1) << "% ";
		sout << t->pid() << ':' << procName << '\n';

		if(sout.bad())
			exitmsg("Write failed");
	}

	return EXIT_SUCCESS;
}

static bool compareThreads(const thread* a,const thread* b) {
	switch(sortcol) {
		case sort::TID:
			// ascending
			return a->tid() < b->tid();
		case sort::STACK:
			// descending
			return b->stackPages() < a->stackPages();
		case sort::STATE:
			// descending
			return b->state() < a->state();
		case sort::PRIO:
			// descending
			return b->prio() < a->prio();
		case sort::CPU:
			// descending
			return b->cycles() < a->cycles();
		case sort::TIME:
			// descending
			return b->runtime() < a->runtime();
		case sort::NAME:
			// ascending
			return a->procName() < b->procName();
	}
	// never reached
	return false;
}
