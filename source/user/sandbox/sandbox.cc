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
#include <esc/env.h>
#include <info/mount.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <usergroup/usergroup.h>
#include <dirent.h>
#include <getopt.h>
#include <algorithm>
#include <string>
#include <vector>

using namespace esc;

enum {
	FL_ALONE	= 1,
	FL_LEAVEALL	= 2,
};

static uint flags;

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

struct Remount {
	explicit Remount() : path(), perm() {
	}
	explicit Remount(const std::string &_path,uint _perm) : path(_path), perm(_perm) {
	}

	std::string path;
	uint perm;
};

static bool operator<(const Remount &r1,const Remount &r2) {
	return r1.path.length() > r2.path.length();
}

static const char *permToStr(uint perm) {
	static char buf[4];
	buf[0] = (perm & O_RDONLY) ? 'r' : '-';
	buf[1] = (perm & O_WRONLY) ? 'w' : '-';
	buf[2] = (perm & O_EXEC) ? 'x' : '-';
	buf[3] = '\0';
	return buf;
}

static uint strToPerm(const char *perms) {
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

static void addMount(std::vector<Remount> &mounts,const char *path,uint perm,bool pref) {
	for(auto &r : mounts) {
		if(strcmp(r.path.c_str(),path) == 0) {
			if(pref)
				r.perm = perm;
			return;
		}
	}
	mounts.push_back(Remount(path,perm));
}

static void addMounts(const std::vector<info::mount*> &pmnts,
		std::vector<Remount> &smnts,const std::string &path,uint perm) {
	char cpath[MAX_PATH_LEN];
	for(auto &m : pmnts) {
		if(strncmp(path.c_str(),m->path().c_str(),path.length()) == 0 && path != m->path()) {
			if(~perm & m->perm()) {
				if(canonpath(cpath,sizeof(cpath),m->path().c_str()) < 0)
					error("canonpath for '%s' failed",m->path().c_str());
				addMount(smnts,cpath,perm & m->perm(),false);
			}
		}
	}

	if(canonpath(cpath,sizeof(cpath),path.c_str()) < 0)
		error("canonpath for '%s' failed",path.c_str());
	addMount(smnts,cpath,perm,true);
}

static void addMounts(const std::vector<info::mount*> &pmnts,
		std::vector<Remount> &smnts,char *spec) {
	char *colon = strchr(spec,':');
	if(!colon)
		error("Invalid mount specification: %s",spec);
	*colon = '\0';
	addMounts(pmnts,smnts,spec,strToPerm(colon + 1));
}

static void remount(const std::vector<info::mount*> pmnts,std::vector<Remount> &mounts) {
	int ms = -1;

	for(int i = 1; ; ++i) {
		char path[MAX_PATH_LEN];
		snprintf(path,sizeof(path),"sandbox-%d",i);
		if(clonems(path) == 0) {
			ms = open("/sys/pid/self/ms",O_RDWR);
			if(ms < 0)
				error("Unable to open mountspace '/sys/pid/self/ms' for writing");
			break;
		}
	}

	// make the hierarchy below our process writable
	{
		esc::OStringStream os;
		os << "/sys/pid/" << getpid();
		addMounts(pmnts,mounts,os.str(),O_RDWR);
		// but everything else readonly / invisible
		addMounts(pmnts,mounts,"/sys/proc",(flags & FL_ALONE) ? O_EXEC : O_RDONLY);
	}

	// make the hierarchy below our mountspace writable
	{
		esc::OStringStream os;
		os << "/sys/pid/" << getpid() << "/ms";
		addMounts(pmnts,mounts,os.str(),O_RDWR);
		// but everything else readonly
		addMounts(pmnts,mounts,"/sys/mount",O_RDONLY);
	}

	// sort them, so that we perform the remounts of the inner paths first
	std::sort(mounts.begin(),mounts.end());

	for(auto m = mounts.begin(); m != mounts.end(); ++m) {
		int dir = open(m->path.c_str(),O_NOCHAN);
		if(dir < 0)
			error("Unable to open '%s'",m->path.c_str());
		if(!fisdir(dir))
			error("'%s' is no directory",m->path.c_str());
		if(::remount(ms,dir,m->perm) < 0)
			error("Remounting '%s' with permissions '%s' failed",m->path.c_str(),permToStr(m->perm));
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
	serr << "\n";
	serr << "The -m option can be specified multiple times. For each one, the permissions are\n";
	serr << "also enforced indirectly on all mounts below <path>. The permissions specified for\n";
	serr << "<path> take precedence over the indirectly enforced permissions. For example, if\n";
	serr << "'/', '/dev' and '/sys' is mounted and you specify '-m /:rw' and '-m /sys:r', the\n";
	serr << "result will be /:rw, /dev:rw, /sys:r, independent of the order of the two -m options.\n";
	exit(1);
}

int main(int argc,char **argv) {
	std::vector<std::string> leaveGroups;
	std::vector<Remount> mounts;

	const std::vector<info::mount*> pmnts = info::mount::get_list();

	// parse args
	int opt;
	while((opt = getopt(argc,argv,"aLl:m:")) != -1) {
		switch(opt) {
			case 'a': flags |= FL_ALONE; break;
			case 'L': flags |= FL_LEAVEALL; break;
			case 'l': leaveGroups.push_back(optarg); break;
			case 'm': addMounts(pmnts,mounts,optarg); break;
			default:
				usage(argv[0]);
		}
	}
	if(optind >= argc)
		usage(argv[0]);

	int pid;
	if((pid = fork()) == 0) {
		// the sandboxed process shouldn't share stdin, stdout and stderr with us.
		const std::string &term = env::get("TERM");
		int maxfds = sysconf(CONF_MAX_FDS);
		for(int i = 0; i < maxfds; ++i) {
			if(i != 3)
				close(i);
		}
		if(open(term.c_str(),O_RDONLY | O_MSGS) != STDIN_FILENO)
			error("Unable to reopen '%s' for stdin",term.c_str());
		if(open(term.c_str(),O_WRONLY | O_MSGS) != STDOUT_FILENO)
			error("Unable to reopen '%s' for stdout",term.c_str());
		if(dup(STDOUT_FILENO) != STDERR_FILENO)
			error("Unable to duplicate stdout to stderr");

		// remounts
		remount(pmnts,mounts);

		// leave groups
		int res = 0;
		if(flags & FL_LEAVEALL)
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
