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

#include <esc/stream/fstream.h>
#include <esc/file.h>
#include <info/process.h>
#include <info/thread.h>
#include <ctype.h>

using namespace esc;

namespace info {
	std::vector<process*> process::get_list(bool own,uid_t uid,bool fullcmd) {
		std::vector<process*> procs;
		file dir("/sys/proc");
		std::vector<struct dirent> files = dir.list_files(false);
		for(auto it = files.begin(); it != files.end(); ++it) {
			/* skip "self" */
			if(!isdigit(it->d_name[0]))
				continue;
			try {
				process *p = get_proc(strtoul(it->d_name,NULL,10),own,uid,fullcmd);
				if(p)
					procs.push_back(p);
			}
			catch(const default_error&) {
			}
		}
		return procs;
	}

	process* process::get_proc(pid_t pid,bool own,uid_t uid,bool fullcmd) {
		char name[12];
		itoa(name,sizeof(name),pid);
		std::string ppath = std::string("/sys/proc/") + name + "/info";
		FStream is(ppath.c_str(),"r");
		if(!is)
			return NULL;
		process* p = new process(fullcmd);
		is >> *p;

		if(own && p->uid() != uid) {
			delete p;
			return NULL;
		}
		return p;
	}

	process::process(const process& p)
		: _fullcmd(p._fullcmd), _pid(p._pid), _ppid(p._ppid), _uid(p._uid), _gid(p._gid),
		  _pages(p._pages), _ownFrames(p._ownFrames), _sharedFrames(p._sharedFrames),
		  _swapped(p._swapped), _cycles(p._cycles), _runtime(p._runtime), _input(p._input),
		  _output(p._output), _cmd(p._cmd) {
	}

	process& process::operator =(const process& p) {
		_fullcmd = p._fullcmd;
		_pid = p._pid;
		_ppid = p._ppid;
		_uid = p._uid;
		_gid = p._gid;
		_pages = p._pages;
		_ownFrames = p._ownFrames;
		_sharedFrames = p._sharedFrames;
		_swapped = p._swapped;
		_cycles = p._cycles;
		_runtime = p._runtime;
		_input = p._input;
		_output = p._output;
		_cmd = p._cmd;
		return *this;
	}

	IStream& operator >>(IStream& is,process& p) {
		size_t unlimited = std::numeric_limits<size_t>::max();
		is.ignore(unlimited,' ') >> p._pid;
		is.ignore(unlimited,' ') >> p._ppid;
		is.ignore(unlimited,' ') >> p._uid;
		is.ignore(unlimited,' ') >> p._gid;
		is.ignore(unlimited,' ');
		is.ignore_ws();
		if(p._fullcmd)
			is.getline(p._cmd,'\n');
		else {
			is >> p._cmd;
			is.ignore(unlimited,'\n');
		}
		is.ignore(unlimited,' ') >> p._pages;
		is.ignore(unlimited,' ') >> p._ownFrames;
		is.ignore(unlimited,' ') >> p._sharedFrames;
		is.ignore(unlimited,' ') >> p._swapped;
		is.ignore(unlimited,' ') >> p._input;
		is.ignore(unlimited,' ') >> p._output;
		is.ignore(unlimited,' ') >> p._runtime;
		is.ignore(unlimited,' ') >> fmt(p._cycles,"x");
		return is;
	}

	OStream& operator <<(OStream& os,const process& p) {
		os << "process[" << p.pid() << ":" << p.command() << "]:\n";
		os << "\tppid      : " << p.ppid() << "\n";
		os << "\tuid       : " << p.uid() << "\n";
		os << "\tgid       : " << p.gid() << "\n";
		os << "\tpages     : " << p.pages() << "\n";
		os << "\townFrames : " << p.ownFrames() << "\n";
		os << "\tshFrames  : " << p.sharedFrames() << "\n";
		os << "\tswpFrames : " << p.swapped() << "\n";
		os << "\tinput     : " << p.input() << "\n";
		os << "\toutput    : " << p.output() << "\n";
		os << "\truntime   : " << p.runtime() << "\n";
		os << "\tcycles    : " << p.cycles() << "\n";
		return os;
	}
}
