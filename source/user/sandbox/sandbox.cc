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

#include <esc/stream/std.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <usergroup/usergroup.h>
#include <dirent.h>
#include <getopt.h>
#include <string>
#include <vector>

using namespace esc;

static int leave(const std::vector<std::string> &groups) {
	// get current groups
	size_t count = getgroups(0,NULL);
	gid_t *curgids = new gid_t[count];
	getgroups(count,curgids);
	std::vector<gid_t> gids(curgids,curgids + count);

	// remove requested groups
	for(auto g = groups.begin(); g != groups.end(); ++g) {
		int gid = usergroup_nameToId(GROUPS_PATH,g->c_str());
		if(gid < 0)
			error("Unable to find group %s",g->c_str());
		else
			gids.erase_first(gid);
	}

	// set the remaining groups
	return setgroups(gids.size(),gids.begin());
}

static uint perms(const char *perms) {
	uint p = 0;
	for(size_t i = 0; perms[i]; ++i) {
		if(perms[i] == 'r')
			p |= O_RDONLY;
		else if(perms[i] == 'w')
			p |= O_WRONLY;
		else if(perms[i] == 'x')
			p |= O_EXEC;
	}
	return p;
}

static void remount(std::vector<std::string> &mounts,bool alone) {
	int ms = -1;

	char path[MAX_PATH_LEN];
	for(int i = 1; ; ++i) {
		snprintf(path,sizeof(path),"sandbox-%d",i);
		if(clonems(path) == 0) {
			ms = open("/sys/pid/self/ms",O_RDWR);
			if(ms < 0)
				error("Unable to open mountspace '/sys/pid/self/ms' for writing");
			break;
		}
	}

	/* make the hierarchy below our process writable */
	{
		esc::OStringStream os;
		os << "/sys/pid/" << getpid();
		if(canonpath(path,sizeof(path),os.str().c_str()) < 0)
			error("canonpath for '%s' failed",os.str().c_str());
		mounts.push_back(std::string(path) + ":rw");
		/* but everything else readonly / invisible */
		mounts.push_back(std::string("/sys/proc:") + (alone ? "x" : "r"));
	}

	/* make the hierarchy below our mountspace writable */
	{
		esc::OStringStream os;
		os << "/sys/pid/" << getpid() << "/ms";
		if(canonpath(path,sizeof(path),os.str().c_str()) < 0)
			error("canonpath for '%s' failed",os.str().c_str());
		mounts.push_back(std::string(path) + ":rw");
		/* but everything else readonly */
		mounts.push_back("/sys/mount:r");
	}

	for(auto m = mounts.begin(); m != mounts.end(); ++m) {
		char *colon = strchr(m->c_str(),':');
		if(!colon)
			error("Invalid mount specification: %s",m->c_str());

		*colon = '\0';
		uint p = perms(colon + 1);

		int dir = open(m->c_str(),O_NOCHAN);
		if(dir < 0)
			error("Unable to open '%s'",m->c_str());
		if(!fisdir(dir))
			error("'%s' is no directory",m->c_str());
		if(::remount(ms,dir,m->c_str(),p) < 0)
			error("Remounting '%s' with permissions '%s' failed",m->c_str(),colon + 1);
		close(dir);
	}

	close(ms);
}

static void usage(const char *name) {
	serr << "Usage: " << name << " [-a] [-L] [-l <group>] [-m <path>:<perms>] <program> [<arg>...]\n";
	serr << "  -a:                hide other processes, so that the sandbox is `alone'.\n";
	serr << "  -L:                leave all groups\n";
	serr << "  -l <group>:        leave given group\n";
	serr << "  -m <path>:<perms>: remount <path> and reduce permissions to <perms> (rwx)\n";
	exit(1);
}

int main(int argc,char **argv) {
	bool alone = false;
	bool leaveAll = false;
	std::vector<std::string> leaveGroups;
	std::vector<std::string> mounts;

	// parse args
	int opt;
	while((opt = getopt(argc,argv,"aLl:m:")) != -1) {
		switch(opt) {
			case 'a': alone = true; break;
			case 'L': leaveAll = true; break;
			case 'l': leaveGroups.push_back(optarg); break;
			case 'm': mounts.push_back(optarg); break;
			default:
				usage(argv[0]);
		}
	}
	if(optind >= argc)
		usage(argv[0]);

	int pid;
	if((pid = fork()) == 0) {
		// remounts
		remount(mounts,alone);

		// leave groups
		int res = 0;
		if(leaveAll)
			res = setgroups(0,NULL);
		else if(!leaveGroups.empty())
			res = leave(leaveGroups);
		if(res < 0)
			error("Unable to leave groups");

		// build arguments
		const char **cargs = new const char*[argc - optind + 1];
		if(!cargs)
			error("Not enough mem");
		for(int i = optind; i < argc; ++i)
			cargs[i - optind] = argv[i];
		cargs[argc - optind] = NULL;

		// go!
		execvp(cargs[0],cargs);
		error("Exec failed");
	}
	else if(pid < 0)
		error("Fork failed");
	else {
		sExitState state;
		waitchild(&state,-1,0);

		sout << "\n";
		sout << "Process " << state.pid << " ('";
		for(int i = optind; i < argc; ++i) {
			sout << argv[i];
			if(i + 1 < argc)
				sout << ' ';
		}
		sout << "') terminated with exit-code " << state.exitCode;
		if(state.signal != SIG_COUNT)
			sout << " (signal " << state.signal << ")";
		sout << "\n";
	}

	return 0;
}
