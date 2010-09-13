/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/width.h>
#include <esc/messages.h>
#include <esc/debug.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmdargs.h>
#include <file.h>

#include "thread.h"
#include "process.h"

struct sort {
	const static int PID	= 0;
	const static int PPID	= 1;
	const static int MEM	= 2;
	const static int CPU	= 3;
	const static int UCPU	= 4;
	const static int KCPU	= 5;
	const static int NAME	= 6;

	int type;
	string name;
};
static struct sort sorts[] = {
	{sort::PID,"pid"},
	{sort::PPID,"ppid"},
	{sort::MEM,"mem"},
	{sort::CPU,"cpu"},
	{sort::UCPU,"ucpu"},
	{sort::KCPU,"kcpu"},
	{sort::NAME,"name"},
};
static const char *states[] = {
	"Ill ",
	"Run ",
	"Rdy ",
	"Blk ",
	"Zom "
};
static int sortcol = sort::PID;

using namespace std;

static bool compareProcs(const process* a,const process* b);
static vector<process*> getProcs();
static process* getProc(const char* name);
static void usage(const char *name) {
	u32 i;
	cerr << "Usage: " << name <<" [-t][-n <count>][-s <sort>]" << endl;
	cerr << "	-t			: Print threads, too" << endl;
	cerr << "	-n <count>	: Print first <count> processes" << endl;
	cerr << "	-s <sort>	: Sort by ";
	for(i = 0; i < ARRAY_SIZE(sorts); i++) {
		cerr << "'" << sorts[i].name << "'";
		if(i < ARRAY_SIZE(sorts) - 1)
			cerr << ", ";
	}
	cerr << endl;
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	string ssort("pid");
	size_t numProcs = 0;
	bool printThreads = false;

	// parse args
	cmdargs args(argc,argv,cmdargs::NO_FREE);
	try {
		args.parse("s=s n=d t",&ssort,&numProcs,&printThreads);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		cerr << "Invalid arguments: " << e.what() << endl;
		usage(argv[0]);
	}

	// determine sort
	for(size_t i = 0; i < ARRAY_SIZE(sorts); i++) {
		if(ssort == sorts[i].name) {
			sortcol = sorts[i].type;
			break;
		}
	}

	vector<process*> procs = getProcs();
	if(numProcs == 0 || numProcs > procs.size())
		numProcs = procs.size();

	// determine max-values (we want to have a min-width here :))
	process::pid_type maxPid = 100;
	process::pid_type maxPpid = 100;
	process::size_type maxPmem = 100;
	process::size_type maxVmem = 100;
	process::size_type maxSmem = 100;
	process::size_type maxShmem = 100;
	process::size_type maxInput = 10000;
	process::size_type maxOutput = 10000;
	process::cycle_type totalCycles = 0;
	for(vector<process*>::const_iterator it = procs.begin(); it != procs.end(); ++it) {
		process *p = *it;
		if(p->pid() > maxPid)
			maxPid = p->pid();
		if(p->ppid() > maxPpid)
			maxPpid = p->ppid();
		if(p->ownFrames() > maxPmem)
			maxPmem = p->ownFrames();
		if(p->sharedFrames() > maxShmem)
			maxShmem = p->sharedFrames();
		if(p->swapped() > maxSmem)
			maxSmem = p->swapped();
		if(p->pages() > maxVmem)
			maxVmem = p->pages();
		if(p->input() > maxInput)
			maxInput = p->input();
		if(p->output() > maxOutput)
			maxOutput = p->output();
		if(printThreads) {
			const vector<thread*>& threads = p->threads();
			for(vector<thread*>::const_iterator tit = threads.begin(); tit != threads.end(); ++tit) {
				thread *t = *tit;
				if(t->tid() > maxPpid)
					maxPpid = t->tid();
			}
		}
		totalCycles += (*it)->userCycles() + (*it)->kernelCycles();
	}
	maxPid = getuwidth(maxPid,10);
	maxPpid = getuwidth(maxPpid,10);
	// display in KiB, its in pages, i.e. 4 KiB blocks
	maxPmem = getuwidth(maxPmem * 4,10);
	maxVmem = getuwidth(maxVmem * 4,10);
	maxSmem = getuwidth(maxSmem * 4,10);
	maxShmem = getuwidth(maxShmem * 4,10);
	// display in KiB, its in bytes
	maxInput = getuwidth(maxInput / 1024,10);
	maxOutput = getuwidth(maxOutput / 1024,10);

	// sort
	std::sort(procs.begin(),procs.end(),compareProcs);

	// print header
	cout.format(
		"%*sPID%*sPPID%*sPMEM%*sSHMEM%*sVMEM%*sSMEM%*sIN%*sOUT STATE  %%CPU (USER,KERNEL) COMMAND\n",
		maxPid - 3,"",maxPpid - 1,"",maxPmem - 2,"",maxShmem - 2,"",maxVmem - 2,"",
		maxSmem - 2,"",maxInput,"",maxOutput - 1,""
	);

	// calc with to the process-command
	size_t width2cmd = 49 + maxPid + maxPmem + maxShmem + maxSmem + maxInput + maxOutput;
	// get console-size
	sVTSize consSize;
	if(recvMsgData(STDIN_FILENO,MSG_VT_GETSIZE,&consSize,sizeof(sVTSize)) < 0)
		error("Unable to determine screensize");

	// print processes (and threads)
	vector<process*>::const_iterator end = procs.begin() + numProcs;
	for(vector<process*>::const_iterator it = procs.begin(); it != end; ++it) {
		process *p = *it;
		process::cycle_type procCycles;
		int userPercent,kernelPercent;
		float cyclePercent;

		procCycles = p->userCycles() + p->kernelCycles();
		cyclePercent = (float)(100. / (totalCycles / (double)procCycles));
		userPercent = (int)(100. / (procCycles / (double)p->userCycles()));
		kernelPercent = (int)(100. / (procCycles / (double)p->kernelCycles()));
		size_t cmdwidth = min(consSize.width - width2cmd,p->command().length());
		string cmd = p->command().substr(0,cmdwidth);
		cout.format("%*u   %*u %*uK  %*uK %*uK %*uK %*uK %*uK -     %4.1f%% (%3d%%,%3d%%)   %s\n",
				maxPid,p->pid(),maxPpid,p->ppid(),
				maxPmem,p->ownFrames() * 4,
				maxShmem,p->sharedFrames() * 4,
				maxVmem,p->pages() * 4,
				maxSmem,p->swapped() * 4,
				maxInput,p->input() / 1024,
				maxOutput,p->output() / 1024,
				cyclePercent,userPercent,kernelPercent,
				cmd.c_str());

		if(printThreads) {
			const vector<thread*>& threads = p->threads();
			for(vector<thread*>::const_iterator tit = threads.begin(); tit != threads.end(); ++tit) {
				thread *t = *tit;
				process::cycle_type threadCycles = t->userCycles() + t->kernelCycles();
				float tcyclePercent = (float)(100. / (totalCycles / (double)threadCycles));
				int tuserPercent = (int)(100. / (threadCycles / (double)t->userCycles()));
				int tkernelPercent = (int)(100. / (threadCycles / (double)t->kernelCycles()));
				cout.format("  %c\xC4%*s%*d%*s%*uK %*uK %s  %4.1f%% (%3d%%,%3d%%)\n",
						(tit + 1 != threads.end()) ? 0xC3 : 0xC0,
						maxPid - 3,"",maxPpid,t->tid(),
						12 + maxPmem + maxShmem + maxVmem + maxSmem,"",
						maxInput,t->input() / 1024,
						maxOutput,t->output() / 1024,
						states[t->state()],tcyclePercent,tuserPercent,tkernelPercent);
			}
		}
	}

	cout << endl;

	return EXIT_SUCCESS;
}

static bool compareProcs(const process* a,const process* b) {
	switch(sortcol) {
		case sort::PID:
			/* ascending */
			return a->pid() < b->pid();
		case sort::PPID:
			/* ascending */
			return a->ppid() < b->ppid();
		case sort::MEM:
			/* descending */
			return b->pages() < a->pages();
		case sort::CPU:
			/* descending */
			return b->totalCycles() < a->totalCycles();
		case sort::UCPU:
			/* descending */
			return b->userCycles() < a->userCycles();
		case sort::KCPU:
			/* descending */
			return b->kernelCycles() < a->kernelCycles();
		case sort::NAME:
			/* ascending */
			return a->command() < b->command();
	}
	/* never reached */
	return false;
}

static vector<process*> getProcs() {
	vector<process*> procs;
	file dir("/system/processes");
	vector<sDirEntry> files = dir.list_files(false);
	for(vector<sDirEntry>::const_iterator it = files.begin(); it != files.end(); ++it) {
		try {
			procs.push_back(getProc(it->name));
		}
		catch(const io_exception& e) {
			cerr << "Unable to read process with pid " << it->name << ": " << e.what() << endl;
		}
	}
	return procs;
}

static process* getProc(const char* name) {
	string ppath = string("/system/processes/") + name + "/info";
	ifstream is(ppath.c_str());
	process* p = new process();
	is >> *p;
	is.close();

	file dir(string("/system/processes/") + name + "/threads");
	vector<sDirEntry> files = dir.list_files(false);
	for(vector<sDirEntry>::const_iterator it = files.begin(); it != files.end(); ++it) {
		string tpath = string("/system/processes/") + name + "/threads/" + it->name + "/info";
		ifstream tis(tpath.c_str());
		thread* t = new thread();
		tis >> *t;
		p->add_thread(t);
	}
	return p;
}
