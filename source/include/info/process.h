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

#pragma once

#include <esc/stream/istream.h>
#include <esc/stream/ostream.h>
#include <limits>
#include <stddef.h>
#include <string>
#include <vector>

namespace info {
	class thread;

	class process {
		friend esc::IStream& operator >>(esc::IStream& is,process& p);

	public:
		typedef int pid_type;
		typedef int uid_type;
		typedef int gid_type;
		typedef size_t size_type;
		typedef unsigned long long cycle_type;
		typedef unsigned long long time_type;

		static std::vector<process*> get_list(bool own = false,uid_t uid = 0,bool fullcmd = false);
		static process* get_proc(pid_t pid,bool own = false,uid_t uid = 0,bool fullcmd = false);

	public:
		process(bool fullcmd = false)
			: _fullcmd(fullcmd), _pid(0), _ppid(0), _uid(0), _gid(0), _pages(0), _ownFrames(0),
			  _sharedFrames(0), _swapped(0), _cycles(0), _runtime(0), _input(0), _output(0),
			  _cmd() {
		}
		process(const process& p);
		process& operator =(const process& p);

		pid_type pid() const {
			return _pid;
		}
		pid_type ppid() const {
			return _ppid;
		}
		uid_type uid() const {
			return _uid;
		}
		gid_type gid() const {
			return _gid;
		}
		size_type pages() const {
			return _pages;
		}
		size_type ownFrames() const {
			return _ownFrames;
		}
		size_type sharedFrames() const {
			return _sharedFrames;
		}
		size_type swapped() const {
			return _swapped;
		}
		cycle_type cycles() const {
			return _cycles;
		}
		cycle_type runtime() const {
			return _runtime;
		}
		size_type input() const {
			return _input;
		}
		size_type output() const {
			return _output;
		}
		const std::string& command() const {
			return _cmd;
		}

	private:
		bool _fullcmd;
		pid_type _pid;
		pid_type _ppid;
		uid_type _uid;
		gid_type _gid;
		size_type _pages;
		size_type _ownFrames;
		size_type _sharedFrames;
		size_type _swapped;
		cycle_type _cycles;
		time_type _runtime;
		size_type _input;
		size_type _output;
		std::string _cmd;
	};

	esc::IStream& operator >>(esc::IStream& is,process& p);
	esc::OStream& operator <<(esc::OStream& os,const process& p);
}
