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

#include <info/thread.h>
#include <fstream>
#include <vector>
#include <file.h>
#include <ctype.h>

using namespace std;

namespace info {
	vector<thread*> thread::get_list() {
		vector<thread*> threads;
		file dir("/sys/proc");
		vector<struct dirent> files = dir.list_files(false);
		for(vector<struct dirent>::const_iterator it = files.begin(); it != files.end(); ++it) {
			/* skip "self" */
			if(!isdigit(it->d_name[0]))
				continue;

			try {
				string threadDir(string("/sys/proc/") + it->d_name + "/threads");
				file tdir(threadDir);
				vector<struct dirent> tfiles = tdir.list_files(false);
				for(vector<struct dirent>::const_iterator tit = tfiles.begin(); tit != tfiles.end(); ++tit) {
					string tpath = threadDir + "/" + tit->d_name + "/info";
					ifstream tis(tpath.c_str());
					thread* t = new thread();
					tis >> *t;
					threads.push_back(t);
				}
			}
			catch(const default_error&) {
			}
		}
		return threads;
	}

	thread *thread::get_thread(pid_t pid,tid_t tid) {
		char tname[12], pname[12];
		itoa(tname,sizeof(tname),tid);
		itoa(pname,sizeof(pname),pid);
		string tpath = string("/sys/proc/") + pname + "/threads/" + tname + "/info";
		ifstream tis(tpath.c_str());
		thread* t = new thread();
		tis >> *t;
		return t;
	}

	std::istream& operator >>(std::istream& is,thread& t) {
		std::istream::size_type unlimited = std::numeric_limits<streamsize>::max();
		is.ignore(unlimited,' ') >> t._tid;
		is.ignore(unlimited,' ') >> t._pid;
		is.ignore(unlimited,' ') >> std::ws;
		is.getline(t._procName,'\n');
		is.ignore(unlimited,' ') >> t._state;
		is.ignore(unlimited,' ') >> t._flags;
		is.ignore(unlimited,' ') >> t._prio;
		is.ignore(unlimited,' ') >> t._stackPages;
		is.ignore(unlimited,' ') >> t._schedCount;
		is.ignore(unlimited,' ') >> t._syscalls;
		is.ignore(unlimited,' ') >> t._runtime;
		is.setf(istream::hex);
		is.ignore(unlimited,' ') >> t._cycles;
		is.setf(istream::dec);
		is.ignore(unlimited,' ') >> t._cpu;
		return is;
	}

	std::ostream& operator <<(std::ostream& os,const thread& t) {
		os << "thread[" << t.tid() << " in " << t.pid() << ":" << t.procName() << "]:\n";
		os << "\tflags     : " << t.flags() << "\n";
		os << "\tstate     : " << t.state() << "\n";
		os << "\tpriority  : " << t.prio() << "\n";
		os << "\tstackPages: " << t.stackPages() << "\n";
		os << "\tschedCount: " << t.schedCount() << "\n";
		os << "\tsyscalls  : " << t.syscalls() << "\n";
		os << "\tcycles    : " << t.cycles() << "\n";
		os << "\truntime   : " << t.runtime() << "\n";
		os << "\tlastCPU   : " << t.cpu() << "\n";
		return os;
	}
}
