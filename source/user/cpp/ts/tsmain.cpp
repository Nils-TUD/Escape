/**
 * $Id: psmain.cpp 965 2011-07-07 10:56:45Z nasmussen $
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

#define MIN_WIDTH_FOR_UIDGID	90

using namespace std;

struct sort {
	const static int TID	= 0;
	const static int NAME	= 2;
	const static int STACK	= 3;
	const static int STATE	= 4;
	const static int UCPU	= 5;
	const static int KCPU	= 6;

	int type;
	string name;
};

static bool compareThreads(const thread* a,const thread* b);
static vector<thread*> getThreads();

static struct sort sorts[] = {
	{sort::TID,"tid"},
	{sort::NAME,"proc"},
	{sort::STACK,"stack"},
	{sort::STATE,"state"},
	{sort::UCPU,"ucpu"},
	{sort::KCPU,"kcpu"},
};
static const char *states[] = {
	"ILL",
	"RUN",
	"RDY",
	"BLK",
	"ZOM",
	"SUS",
	"SUS",
	"SUS",
};
static int sortcol = sort::TID;
static size_t pageSize;

static void usage(const char *name) {
	size_t i;
	cerr << "Usage: " << name <<" [-s <sort>]" << '\n';
	cerr << "	<sort> can be ";
	for(i = 0; i < ARRAY_SIZE(sorts); i++) {
		cerr << "'" << sorts[i].name << "'";
		if(i < ARRAY_SIZE(sorts) - 1)
			cerr << ", ";
	}
	cerr << '\n';
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	string ssort("tid");
	pageSize = getConf(CONF_PAGE_SIZE);

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

	vector<thread*> threads = getThreads();

	// determine max-values (we want to have a min-width here :))
	thread::tid_type maxTid = 10;
	thread::size_type maxStack = 10000;
	thread::size_type maxScheds = 10000;
	thread::size_type maxSyscalls = 1000;
	thread::cycle_type totalCycles = 0;
	for(vector<thread*>::const_iterator it = threads.begin(); it != threads.end(); ++it) {
		thread *t = *it;
		if(t->tid() > maxTid)
			maxTid = t->tid();
		if(t->stackPages() > maxStack)
			maxStack = t->stackPages();
		if(t->schedCount() > maxScheds)
			maxScheds = t->schedCount();
		if(t->syscalls() > maxSyscalls)
			maxSyscalls = t->syscalls();
		totalCycles += t->userCycles() + t->kernelCycles();
	}
	maxTid = count_digits(maxTid,10);
	// display in KiB, its in pages
	maxStack = count_digits((maxStack * pageSize) / 1024,10);
	maxScheds = count_digits(maxScheds,10);
	maxSyscalls = count_digits(maxSyscalls,10);

	// sort
	std::sort(threads.begin(),threads.end(),compareThreads);

	// print header
	cout << right;
	cout << setw(maxTid) << "ID";
	cout << " STATE";
	cout << setw(maxStack + 1) << "STACK";
	cout << setw(maxScheds + 1) << "SCHED";
	cout << setw(maxSyscalls + 1) << "SYSC";
	cout << "    CPU (USER,KERNEL) PROCESS" << '\n';

	// print threads
	for(vector<thread*>::const_iterator it = threads.begin(); it != threads.end(); ++it) {
		thread *t = *it;
		thread::cycle_type threadCycles;
		int userPercent = 0;
		int kernelPercent = 0;
		float cyclePercent = 0;

		threadCycles = t->userCycles() + t->kernelCycles();
		if(threadCycles != 0)
			cyclePercent = (float)(100. / (totalCycles / (double)threadCycles));
		if(t->userCycles() != 0)
			userPercent = (int)(100. / (threadCycles / (double)t->userCycles()));
		if(t->kernelCycles() != 0)
			kernelPercent = (int)(100. / (threadCycles / (double)t->kernelCycles()));

		cout << setw(maxTid) << t->tid() << " ";
		cout << setw(5) << states[t->state()] << " ";
		cout << setw(maxStack - 1) << (t->stackPages() * pageSize) / 1024 << "K ";
		cout << setw(maxScheds) << t->schedCount() << " ";
		cout << setw(maxSyscalls) << t->syscalls() << " ";
		cout << setw(5) << setprecision(1) << cyclePercent << "% (";
		cout << setw(3) << userPercent << "%,";
		cout << setw(3) << kernelPercent << "%)   ";
		cout << t->pid() << ':' << t->procName() << '\n';
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
		case sort::UCPU:
			// descending
			return b->userCycles() < a->userCycles();
		case sort::KCPU:
			// descending
			return b->kernelCycles() < a->kernelCycles();
		case sort::NAME:
			// ascending
			return a->procName() < b->procName();
	}
	// never reached
	return false;
}

static vector<thread*> getThreads() {
	vector<thread*> threads;
	file dir("/system/processes");
	vector<sDirEntry> files = dir.list_files(false);
	for(vector<sDirEntry>::const_iterator it = files.begin(); it != files.end(); ++it) {
		try {
			string threadDir(string("/system/processes/") + it->name + "/threads");
			file tdir(threadDir);
			vector<sDirEntry> tfiles = tdir.list_files(false);
			for(vector<sDirEntry>::const_iterator tit = tfiles.begin(); tit != tfiles.end(); ++tit) {
				string tpath = threadDir + "/" + tit->name + "/info";
				ifstream tis(tpath.c_str());
				thread* t = new thread();
				tis >> *t;
				threads.push_back(t);
			}
		}
		catch(const io_exception& e) {
			cerr << "Unable to read process with pid " << it->name << ": " << e.what() << endl;
		}
	}
	return threads;
}
