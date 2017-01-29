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
#include <sys/proc.h>
#include <usergroup/usergroup.h>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace esc;
using namespace info;

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

static void usage(const char *name) {
	size_t i;
	serr << "Usage: " << name <<" [-u][-s <sort>]" << '\n';
	serr << "    -u: print only the processes with my uid\n";
	serr << "    -n: print numeric user- and group-ids\n";
	serr << "    -s: sort by ";
	for(i = 0; i < ARRAY_SIZE(sorts); i++) {
		serr << "'" << sorts[i].name << "'";
		if(i < ARRAY_SIZE(sorts) - 1)
			serr << ", ";
	}
	serr << '\n';
	serr << '\n';
	serr << "Explanation of the displayed information:\n";
	serr << "ID    : the process id\n";
	serr << "PID   : the process id of the parent process\n";
	serr << "USER  : the user name\n";
	serr << "GROUP : the group name\n";
	serr << "PMEM  : the amount of physical memory the process uses on its own (not shared)\n";
	serr << "SHMEM : the amount of physical memory the process shares with other processes\n";
	serr << "SMEM  : the amount of physical memory that is currently swapped out\n";
	serr << "IN    : the amount of data read from other processes (read,receive)\n";
	serr << "OUT   : the amount of data written to other processes (write,send)\n";
	serr << "TIME  : the CPU time used by the process so far in minutes:seconds.milliseconds\n";
	serr << "CPU   : the CPU usage in the last second in percent\n";
	serr << "NAME  : the name of the process\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	string ssort("pid");
	int own = 0,numeric = 0;
	sNamedItem *userList = nullptr;
	sNamedItem *groupList = nullptr;

	// parse args
	esc::cmdargs args(argc,argv,esc::cmdargs::NO_FREE);
	try {
		args.parse("s=s u n",&ssort,&own,&numeric);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const esc::cmdargs_error& e) {
		serr << "Invalid arguments: " << e.what() << '\n';
		usage(argv[0]);
	}

	// determine sort
	for(size_t i = 0; i < ARRAY_SIZE(sorts); i++) {
		if(ssort == sorts[i].name) {
			sortcol = sorts[i].type;
			break;
		}
	}

	/* parse users and groups from file */
	userList = usergroup_parse(USERS_PATH,nullptr);
	if(!userList) {
		numeric = true;
		printe("Warning: unable to parse users from file");
	}
	groupList = usergroup_parse(GROUPS_PATH,nullptr);
	if(!groupList) {
		numeric = true;
		printe("Warning: unable to parse groups from file");
	}

	uid_t uid = own ? getuid() : 0;
	vector<process*> procs = process::get_list(false,0,true);

	// determine max-values (we want to have a min-width here :))
	process::pid_type maxPid = 10;
	process::pid_type maxPpid = 100;
	process::uid_type maxUid = 4;
	process::gid_type maxGid = 5;
	process::size_type maxPmem = (10 * PAGE_SIZE) / 1024;
	process::size_type maxShmem = (100 * PAGE_SIZE) / 1024;
	process::size_type maxSmem = (10 * PAGE_SIZE) / 1024;
	process::size_type maxInput = 10 * 1024;
	process::size_type maxOutput = 10 * 1024;
	process::size_type maxRuntime = 100;
	process::cycle_type totalCycles = 0;
	for(auto it = procs.begin(); it != procs.end(); ++it) {
		size_t x;
		process *p = *it;
		if(!own || uid == p->uid()) {
			if(p->pid() > maxPid)
				maxPid = p->pid();
			if(p->ppid() > maxPpid)
				maxPpid = p->ppid();

			sNamedItem *u = !numeric ? usergroup_getById(userList,p->uid()) : nullptr;
			if(!u || numeric) {
				if((x = count_digits((ulong)p->uid(),10)) > (size_t)maxUid)
					maxUid = x;
			}
			else if((x = strlen(u->name)) > (size_t)maxUid)
				maxUid = x;

			sNamedItem *g = !numeric ? usergroup_getById(groupList,p->gid()) : nullptr;
			if(!g || numeric) {
				if((x = count_digits((ulong)p->gid(),10)) > (size_t)maxGid)
					maxGid = x;
			}
			else if((x = strlen(g->name)) > (size_t)maxGid)
				maxGid = x;

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
		}
		totalCycles += p->cycles();
	}
	maxPid = count_digits(maxPid,10);
	maxPpid = count_digits(maxPpid,10);
	// display in KiB, its in pages
	maxPmem = count_digits((maxPmem * PAGE_SIZE) / 1024,10);
	maxSmem = count_digits((maxSmem * PAGE_SIZE) / 1024,10);
	maxShmem = count_digits((maxShmem * PAGE_SIZE) / 1024,10);
	// display in KiB, its in bytes
	maxInput = count_digits(maxInput / 1024,10);
	maxOutput = count_digits(maxOutput / 1024,10);
	maxRuntime = count_digits(maxRuntime / (1000000 * 60),10);

	// sort
	std::sort(procs.begin(),procs.end(),compareProcs);

	// get console-size
	uint cols = VTerm::getSize(env::get("TERM").c_str()).first;

	// print header
	sout << fmt("ID", maxPid);
	sout << " " << fmt("PID",maxPpid);
	sout << " " << fmt("USER","-",maxUid);
	sout << " " << fmt("GROUP","-",maxGid);
	sout << " " << fmt("PMEM",maxPmem + 1);
	sout << " " << fmt("SHMEM",maxShmem + 1);
	sout << " " << fmt("SMEM",maxSmem + 1);
	sout << " " << fmt("IN",maxInput + 1);
	sout << " " << fmt("OUT",maxOutput + 1);
	sout << " " << fmt("TIME",maxRuntime + 7);
	sout << "   CPU NAME" << '\n';

	// calc with to the process-command
	size_t width2cmd = maxPid + maxPpid + maxGid + maxUid + maxPmem + maxShmem + maxSmem +
			maxInput + maxOutput + maxRuntime;
	width2cmd += 10 + 5 + 7 + SSTRLEN("  CPU ");

	// print processes (and threads)
	for(auto it = procs.begin(); it != procs.end(); ++it) {
		process *p = *it;
		if(!own || uid == p->uid()) {
			float cyclePercent = 0;
			if(p->cycles() != 0)
				cyclePercent = (float)(100. / (totalCycles / (double)p->cycles()));
			size_t cmdwidth = min(cols - width2cmd,p->command().length());
			string cmd = p->command().substr(0,cmdwidth);

			sout << fmt(p->pid(),maxPid) << " ";
			sout << fmt(p->ppid(),maxPpid) << " ";

			sNamedItem *u = !numeric ? usergroup_getById(userList,p->uid()) : nullptr;
			if(!u || numeric)
				sout << fmt(p->uid(),"-",maxUid) << " ";
			else
				sout << fmt(u->name,"-",maxUid) << " ";

			sNamedItem *g = !numeric ? usergroup_getById(groupList,p->gid()) : nullptr;
			if(!g || numeric)
				sout << fmt(p->gid(),"-",maxGid) << " ";
			else
				sout << fmt(g->name,"-",maxGid) << " ";

			sout << fmt((p->ownFrames() * PAGE_SIZE) / 1024,maxPmem) << "K ";
			sout << fmt((p->sharedFrames() * PAGE_SIZE) / 1024,maxShmem) << "K ";
			sout << fmt((p->swapped() * PAGE_SIZE) / 1024,maxSmem) << "K ";
			sout << fmt(p->input() / 1024,maxInput) << "K ";
			sout << fmt(p->output() / 1024,maxOutput) << "K ";
			process::time_type time = p->runtime() / 1000;
			sout << fmt(time / (1000 * 60),maxRuntime) << ":";
			time %= 1000 * 60;
			sout << fmt(time / 1000,"0",2) << ".";
			time %= 1000;
			sout << fmt(time,"0",3) << " ";
			sout << fmt(cyclePercent,4,1) << "% ";
			sout << cmd << '\n';

			if(sout.bad())
				error("Write failed");
		}
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
