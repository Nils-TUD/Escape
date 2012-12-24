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

#include <esc/common.h>
#include <esc/driver/vterm.h>
#include <esc/messages.h>
#include <esc/debug.h>
#include <esc/conf.h>
#include <proc/process.h>
#include <proc/thread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmdargs.h>
#include <file.h>

#define MIN_WIDTH_FOR_UIDGID	90

using namespace std;
using namespace proc;

struct sort {
	const static int PID	= 0;
	const static int PPID	= 1;
	const static int MEM	= 2;
	const static int CPU	= 3;
	const static int TIME	= 4;
	const static int NAME	= 5;

	int type;
	string name;
};

static bool compareProcs(const process* a,const process* b);

static struct sort sorts[] = {
	{sort::PID,"pid"},
	{sort::PPID,"ppid"},
	{sort::MEM,"mem"},
	{sort::CPU,"cpu"},
	{sort::TIME,"time"},
	{sort::NAME,"name"},
};
static int sortcol = sort::PID;
static size_t pageSize;

static void usage(const char *name) {
	size_t i;
	cerr << "Usage: " << name <<" [-u][-s <sort>]" << '\n';
	cerr << "	-u		Print only the processes with my uid\n";
	cerr << "	-s		Sort by ";
	for(i = 0; i < ARRAY_SIZE(sorts); i++) {
		cerr << "'" << sorts[i].name << "'";
		if(i < ARRAY_SIZE(sorts) - 1)
			cerr << ", ";
	}
	cerr << '\n';
	cerr << '\n';
	cerr << "Explanation of the displayed information:\n";
	cerr << "ID:		The process id\n";
	cerr << "PID:	The process id of the parent process\n";
	cerr << "UID:	The user id\n";
	cerr << "GID:	The group id\n";
	cerr << "PMEM:	The amount of physical memory the process uses on its own (not shared)\n";
	cerr << "SHMEM:	The amount of physical memory the process shares with other processes\n";
	cerr << "SMEM:	The amount of physical memory that is currently swapped out\n";
	cerr << "IN:		The amount of data read from other processes (read,receive)\n";
	cerr << "OUT:	The amount of data written to other processes (write,send)\n";
	cerr << "TIME:	The CPU time used by the process so far in minutes:seconds.milliseconds\n";
	cerr << "CPU:	The CPU usage in the last second in percent\n";
	cerr << "NAME:	The name of the process\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	string ssort("pid");
	pageSize = sysconf(CONF_PAGE_SIZE);
	bool own = false;

	// parse args
	cmdargs args(argc,argv,cmdargs::NO_FREE);
	try {
		args.parse("s=s u",&ssort,&own);
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

	uid_t uid = own ? getuid() : 0;
	vector<process*> procs = process::get_list(own,uid);

	// determine max-values (we want to have a min-width here :))
	process::pid_type maxPid = 10;
	process::pid_type maxPpid = 100;
	process::uid_type maxUid = 100;
	process::gid_type maxGid = 100;
	process::size_type maxPmem = (10 * pageSize) / 1024;
	process::size_type maxShmem = (100 * pageSize) / 1024;
	process::size_type maxSmem = (10 * pageSize) / 1024;
	process::size_type maxInput = 10 * 1024;
	process::size_type maxOutput = 10 * 1024;
	process::size_type maxRuntime = 100;
	process::cycle_type totalCycles = 0;
	for(vector<process*>::const_iterator it = procs.begin(); it != procs.end(); ++it) {
		process *p = *it;
		if(p->pid() > maxPid)
			maxPid = p->pid();
		if(p->ppid() > maxPpid)
			maxPpid = p->ppid();
		if(p->uid() > maxUid)
			maxUid = p->uid();
		if(p->gid() > maxGid)
			maxGid = p->gid();
		if(p->ownFrames() > maxPmem)
			maxPmem = p->ownFrames();
		if(p->sharedFrames() > maxShmem)
			maxShmem = p->sharedFrames();
		if(p->swapped() > maxSmem)
			maxSmem = p->swapped();
		if(p->input() > maxInput)
			maxInput = p->input();
		if(p->output() > maxOutput)
			maxOutput = p->output();
		if(p->runtime() > maxRuntime)
			maxRuntime = p->runtime();
		totalCycles += p->cycles();
	}
	maxPid = count_digits(maxPid,10);
	maxPpid = count_digits(maxPpid,10);
	maxUid = count_digits(maxUid,10);
	maxGid = count_digits(maxGid,10);
	// display in KiB, its in pages
	maxPmem = count_digits((maxPmem * pageSize) / 1024,10);
	maxSmem = count_digits((maxSmem * pageSize) / 1024,10);
	maxShmem = count_digits((maxShmem * pageSize) / 1024,10);
	// display in KiB, its in bytes
	maxInput = count_digits(maxInput / 1024,10);
	maxOutput = count_digits(maxOutput / 1024,10);
	maxRuntime = count_digits(maxRuntime / (1000000 * 60),10);

	// sort
	std::sort(procs.begin(),procs.end(),compareProcs);

	// get console-size
	sVTSize consSize;
	if(vterm_getSize(STDIN_FILENO,&consSize) < 0)
		error("Unable to determine screensize");

	// print header
	cout << setw(maxPid) << "ID";
	cout << setw(maxPpid + 1) << right << "PID";
	cout << setw(maxUid + 1) << right << "UID";
	cout << setw(maxGid + 1) << right << "GID";
	cout << setw((streamsize)maxPmem + 2) << right << "PMEM";
	cout << setw((streamsize)maxShmem + 2) << right << "SHMEM";
	cout << setw((streamsize)maxSmem + 2) << right << "SMEM";
	cout << setw((streamsize)maxInput + 2) << right << "IN";
	cout << setw((streamsize)maxOutput + 2) << right << "OUT";
	cout << setw((streamsize)maxRuntime + 8) << right << "TIME";
	cout << "   CPU NAME" << '\n';

	// calc with to the process-command
	size_t width2cmd = maxPid + maxPpid + maxGid + maxUid + maxPmem + maxShmem + maxSmem +
			maxInput + maxOutput + maxRuntime;
	width2cmd += 2 * 5 + 4 + SSTRLEN("  CPU ");

	// print processes (and threads)
	for(vector<process*>::const_iterator it = procs.begin(); it != procs.end(); ++it) {
		process *p = *it;
		float cyclePercent = 0;
		if(p->cycles() != 0)
			cyclePercent = (float)(100. / (totalCycles / (double)p->cycles()));
		size_t cmdwidth = min(consSize.width - width2cmd,p->command().length());
		string cmd = p->command().substr(0,cmdwidth);

		cout << setw(maxPid) << p->pid() << " ";
		cout << setw(maxPpid) << p->ppid() << " ";
		cout << setw(maxUid) << p->uid() << " ";
		cout << setw(maxGid) << p->gid() << " ";
		cout << setw((streamsize)maxPmem) << (p->ownFrames() * pageSize) / 1024 << "K ";
		cout << setw((streamsize)maxShmem) << (p->sharedFrames() * pageSize) / 1024 << "K ";
		cout << setw((streamsize)maxSmem) << (p->swapped() * pageSize) / 1024 << "K ";
		cout << setw((streamsize)maxInput) << p->input() / 1024 << "K ";
		cout << setw((streamsize)maxOutput) << p->output() / 1024 << "K ";
		process::time_type time = p->runtime() / 1000;
		cout << setw(maxRuntime) << time / (1000 * 60) << ":";
		time %= 1000 * 60;
		cout << setfillobj('0');
		cout << setw(2) << time / 1000 << ".";
		time %= 1000;
		cout << setw(3) << time << " ";
		cout << setfillobj(' ');
		cout << setw(4) << setprecision(1) << cyclePercent << "% ";
		cout << cmd << '\n';
	}

	return EXIT_SUCCESS;
}

static bool compareProcs(const process* a,const process* b) {
	switch(sortcol) {
		case sort::PID:
			// ascending
			return a->pid() < b->pid();
		case sort::PPID:
			// ascending
			return a->ppid() < b->ppid();
		case sort::MEM:
			// descending
			return b->pages() < a->pages();
		case sort::CPU:
			// descending
			return b->cycles() < a->cycles();
		case sort::TIME:
			// descending
			return b->runtime() < a->runtime();
		case sort::NAME:
			// ascending
			return a->command() < b->command();
	}
	// never reached
	return false;
}
