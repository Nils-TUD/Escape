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
#include <esc/messages.h>
#include <esc/debug.h>
#include <esc/conf.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmdargs.h>
#include <file.h>

#include "thread.h"
#include "process.h"

#define MIN_WIDTH_FOR_UIDGID	90

using namespace std;

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

static bool compareProcs(const process* a,const process* b);
static vector<process*> getProcs(bool own,uid_t uid);
static process* getProc(const char* name,bool own,uid_t uid);

static struct sort sorts[] = {
	{sort::PID,"pid"},
	{sort::PPID,"ppid"},
	{sort::MEM,"mem"},
	{sort::CPU,"cpu"},
	{sort::UCPU,"ucpu"},
	{sort::KCPU,"kcpu"},
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
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	string ssort("pid");
	pageSize = getConf(CONF_PAGE_SIZE);
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
	vector<process*> procs = getProcs(own,uid);

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
		totalCycles += p->userCycles() + p->kernelCycles();
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

	// sort
	std::sort(procs.begin(),procs.end(),compareProcs);

	// get console-size
	sVTSize consSize;
	if(recvMsgData(STDIN_FILENO,MSG_VT_GETSIZE,&consSize,sizeof(sVTSize)) < 0)
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
	cout << "   CPU (USER,KERNEL) COMMAND" << '\n';

	// calc with to the process-command
	size_t width2cmd = maxPid + maxPpid + maxGid + maxUid + maxPmem + maxShmem + maxSmem +
			maxInput + maxOutput;
	width2cmd += 2 * 5 + 4 + SSTRLEN("  CPU (USER,KERNEL) ");

	// print processes (and threads)
	for(vector<process*>::const_iterator it = procs.begin(); it != procs.end(); ++it) {
		process *p = *it;
		process::cycle_type procCycles;
		int userPercent = 0;
		int kernelPercent = 0;
		float cyclePercent = 0;

		procCycles = p->userCycles() + p->kernelCycles();
		if(procCycles != 0)
			cyclePercent = (float)(100. / (totalCycles / (double)procCycles));
		if(p->userCycles() != 0)
			userPercent = (int)(100. / (procCycles / (double)p->userCycles()));
		if(p->kernelCycles() != 0)
			kernelPercent = (int)(100. / (procCycles / (double)p->kernelCycles()));
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
		cout << setw(4) << setprecision(1) << cyclePercent << "% (";
		cout << setw(3) << userPercent << "%,";
		cout << setw(3) << kernelPercent << "%)   ";
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
			return b->totalCycles() < a->totalCycles();
		case sort::UCPU:
			// descending
			return b->userCycles() < a->userCycles();
		case sort::KCPU:
			// descending
			return b->kernelCycles() < a->kernelCycles();
		case sort::NAME:
			// ascending
			return a->command() < b->command();
	}
	// never reached
	return false;
}

static vector<process*> getProcs(bool own,uid_t uid) {
	vector<process*> procs;
	file dir("/system/processes");
	vector<sDirEntry> files = dir.list_files(false);
	for(vector<sDirEntry>::const_iterator it = files.begin(); it != files.end(); ++it) {
		try {
			process *p = getProc(it->name,own,uid);
			if(p)
				procs.push_back(p);
		}
		catch(const io_exception& e) {
			cerr << "Unable to read process with pid " << it->name << ": " << e.what() << endl;
		}
	}
	return procs;
}

static process* getProc(const char* name,bool own,uid_t uid) {
	string ppath = string("/system/processes/") + name + "/info";
	ifstream is(ppath.c_str());
	process* p = new process();
	is >> *p;
	is.close();

	if(own && p->uid() != uid) {
		delete p;
		return NULL;
	}

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
