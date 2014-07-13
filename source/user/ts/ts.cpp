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

#include <sys/common.h>
#include <sys/arch.h>
#include <sys/messages.h>
#include <sys/debug.h>
#include <sys/conf.h>
#include <esc/proto/vterm.h>
#include <info/thread.h>
#include <info/process.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmdargs.h>
#include <file.h>
#include <env.h>

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
	cerr << "Usage: " << name <<" [-s <sort>]" << '\n';
	cerr << "    <sort> can be ";
	for(i = 0; i < ARRAY_SIZE(sorts); i++) {
		cerr << "'" << sorts[i].name << "'";
		if(i < ARRAY_SIZE(sorts) - 1)
			cerr << ", ";
	}
	cerr << '\n';
	cerr << '\n';
	cerr << "Explanation of the displayed information:\n";
	cerr << "ID    : the thread id\n";
	cerr << "STATE : thread state (RUN,RDY,BLK,ZOM,SUS)\n";
	cerr << "PRIO  : the thread priority (high means high priority)\n";
	cerr << "CPU   : the CPU on which the thread is currently running on\n";
	cerr << "STACK : the amount of physical memory used for the stack\n";
	cerr << "SCHED : the number of times the thread has been scheduled\n";
	cerr << "SYSC  : the number of system-calls the thread has executed\n";
	cerr << "TIME  : the CPU time used by the thread so far in minutes:seconds.milliseconds\n";
	cerr << "CPU%  : the CPU usage in the last second in percent\n";
	cerr << "PROC  : the name of the process the thread belongs to\n";
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
		cerr << "Invalid arguments: " << e.what() << '\n';
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
	esc::VTerm vterm(std::env::get("TERM").c_str());
	esc::Screen::Mode mode = vterm.getMode();

	// print header
	cout << right;
	cout << setw(maxTid) << "ID";
	cout << " " << "STATE";
	cout << " " << "PRIO";
	cout << " " << " CPU";
	cout << " " << setw(maxStack) << "STACK";
	cout << " " << setw(maxScheds) << "SCHED";
	cout << " " << setw(maxSyscalls) << "SYSC";
	cout << " " << setw(maxRuntime + 7) << "TIME";
	cout << "   CPU% PROC" << '\n';

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

		cout << setw(maxTid) << t->tid() << " ";
		cout << setw(5) << states[t->state()] << " ";
		cout << setw(4) << t->prio() << " ";
		cout << setw(4);
		if(t->state() == 0)
			cout << t->cpu();
		else
			cout << '-';
		cout << " ";
		cout << setw(maxStack - 1) << (t->stackPages() * PAGE_SIZE) / 1024 << "K ";
		cout << setw(maxScheds) << t->schedCount() << " ";
		cout << setw(maxSyscalls) << t->syscalls() << " ";
		thread::time_type time = t->runtime() / 1000;
		cout << setw(maxRuntime) << time / (1000 * 60) << ":";
		time %= 1000 * 60;
		cout << setfillobj('0');
		cout << setw(2) << time / 1000 << ".";
		time %= 1000;
		cout << setw(3) << time << " ";
		cout << setfillobj(' ');
		cout << setw(5) << setprecision(1) << cyclePercent << "% ";
		cout << t->pid() << ':' << procName << '\n';

		if(cout.bad())
			error("Write failed");
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
