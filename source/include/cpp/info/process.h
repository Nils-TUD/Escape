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

#pragma once

#include <stddef.h>
#include <vector>
#include <string>
#include <limits>

namespace info {
	class thread;

	class process {
		friend std::istream& operator >>(std::istream& is,process& p);

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
			  _sharedFrames(0), _swapped(0), _input(0), _output(0), _cycles(-1), _runtime(-1),
			  _threads(), _cmd() {
		}
		process(const process& p);
		process& operator =(const process& p);
		~process() {
			destroy();
		}

		inline pid_type pid() const {
			return _pid;
		};
		inline pid_type ppid() const {
			return _ppid;
		};
		inline uid_type uid() const {
			return _uid;
		};
		inline gid_type gid() const {
			return _gid;
		};
		inline size_type pages() const {
			return _pages;
		};
		inline size_type ownFrames() const {
			return _ownFrames;
		};
		inline size_type sharedFrames() const {
			return _sharedFrames;
		};
		inline size_type swapped() const {
			return _swapped;
		};
		cycle_type cycles() const;
		cycle_type runtime() const;
		inline size_type input() const {
			return _input;
		};
		inline size_type output() const {
			return _output;
		};
		inline const std::vector<thread*>& threads() const {
			return _threads;
		};
		inline void add_thread(thread* t) {
			_threads.push_back(t);
			// we need to refresh that afterwards
			_cycles = -1;
			_runtime = -1;
		};
		inline const std::string& command() const {
			return _cmd;
		};

	private:
		void destroy();

		bool _fullcmd;
		pid_type _pid;
		pid_type _ppid;
		uid_type _uid;
		gid_type _gid;
		size_type _pages;
		size_type _ownFrames;
		size_type _sharedFrames;
		size_type _swapped;
		mutable size_type _input;
		mutable size_type _output;
		mutable cycle_type _cycles;
		mutable time_type _runtime;
		std::vector<thread*> _threads;
		std::string _cmd;
	};

	std::istream& operator >>(std::istream& is,process& p);
	std::ostream& operator <<(std::ostream& os,const process& p);
}
