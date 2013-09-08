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

#include <info/process.h>
#include <info/thread.h>
#include <fstream>
#include <file.h>
#include <ctype.h>

using namespace std;

namespace info {
	vector<process*> process::get_list(bool own,uid_t uid,bool fullcmd) {
		vector<process*> procs;
		file dir("/system/processes");
		vector<sDirEntry> files = dir.list_files(false);
		for(vector<sDirEntry>::const_iterator it = files.begin(); it != files.end(); ++it) {
			/* skip "self" */
			if(!isdigit(it->name[0]))
				continue;
			try {
				process *p = get_proc(strtoul(it->name,NULL,10),own,uid,fullcmd);
				if(p)
					procs.push_back(p);
			}
			catch(const io_exception&) {
			}
		}
		return procs;
	}

	process* process::get_proc(pid_t pid,bool own,uid_t uid,bool fullcmd) {
		char name[12];
		itoa(name,sizeof(name),pid);
		string ppath = string("/system/processes/") + name + "/info";
		ifstream is(ppath.c_str());
		process* p = new process(fullcmd);
		is >> *p;
		is.close();

		if(own && p->uid() != uid) {
			delete p;
			return NULL;
		}

		file dir(string("/system/processes/") + name + "/threads");
		vector<sDirEntry> files = dir.list_files(false);
		for(vector<sDirEntry>::const_iterator it = files.begin(); it != files.end(); ++it) {
			thread *t = thread::get_thread(pid,strtoul(it->name,NULL,10));
			p->add_thread(t);
		}
		return p;
	}

	process::process(const process& p)
		: _fullcmd(p._fullcmd), _pid(p._pid), _ppid(p._ppid), _uid(p._uid), _gid(p._gid),
		  _pages(p._pages), _ownFrames(p._ownFrames), _sharedFrames(p._sharedFrames),
		  _swapped(p._swapped), _input(p._input), _output(p._output), _cycles(p._cycles),
		  _runtime(p._runtime), _threads(), _cmd(p._cmd) {
		for(std::vector<thread*>::const_iterator it = p._threads.begin(); it != p._threads.end(); ++it)
			_threads.push_back(new thread(**it));
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
		destroy();
		_threads.clear();
		for(std::vector<thread*>::const_iterator it = p._threads.begin(); it != p._threads.end(); ++it)
			_threads.push_back(new thread(**it));
		return *this;
	}

	void process::destroy() {
		for(std::vector<thread*>::iterator it = _threads.begin(); it != _threads.end(); ++it)
			delete *it;
	}

	process::cycle_type process::cycles() const {
		if(_cycles == (cycle_type)-1) {
			_cycles = 0;
			for(vector<thread*>::const_iterator it = _threads.begin(); it != _threads.end(); ++it)
				_cycles += (*it)->cycles();
		}
		return _cycles;
	}
	process::cycle_type process::runtime() const {
		if(_runtime == (time_type)-1) {
			_runtime = 0;
			for(vector<thread*>::const_iterator it = _threads.begin(); it != _threads.end(); ++it)
				_runtime += (*it)->runtime();
		}
		return _runtime;
	}

	istream& operator >>(istream& is,process& p) {
		istream::size_type unlimited = numeric_limits<streamsize>::max();
		is.ignore(unlimited,' ') >> p._pid;
		is.ignore(unlimited,' ') >> p._ppid;
		is.ignore(unlimited,' ') >> p._uid;
		is.ignore(unlimited,' ') >> p._gid;
		is.ignore(unlimited,' ');
		if(p._fullcmd) {
			is >> std::ws;
			is.getline(p._cmd,'\n');
		}
		else {
			is >> p._cmd;
			is.unget();
			if(is.peek() != '\n')
				is.ignore(unlimited,'\n');
		}
		is.ignore(unlimited,' ') >> p._pages;
		is.ignore(unlimited,' ') >> p._ownFrames;
		is.ignore(unlimited,' ') >> p._sharedFrames;
		is.ignore(unlimited,' ') >> p._swapped;
		is.ignore(unlimited,' ') >> p._input;
		is.ignore(unlimited,' ') >> p._output;
		return is;
	}

	std::ostream& operator <<(std::ostream& os,const process& p) {
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
		os << "\tthreads   : " << p.threads().size() << "\n";
		os << "\truntime   : " << p.runtime() << "\n";
		os << "\tcycles    : " << p.cycles() << "\n";
		return os;
	}
}
