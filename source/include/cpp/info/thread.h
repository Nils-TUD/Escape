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

#include <istream>

namespace info {
	class thread {
		friend std::istream& operator >>(std::istream& is,thread& t);

	public:
		enum Flags {
			FL_IDLE		= 1
		};

		typedef int pid_type;
		typedef int tid_type;
		typedef int state_type;
		typedef unsigned flags_type;
		typedef int prio_type;
		typedef unsigned cpu_type;
		typedef size_t size_type;
		typedef unsigned long long cycle_type;
		typedef unsigned long long time_type;

		static vector<thread*> get_list();
		static thread *get_thread(pid_t pid,tid_t tid);

	public:
		thread()
			: _tid(0), _pid(0), _procName(std::string()), _state(0), _flags(), _prio(0),
			  _stackPages(0), _schedCount(0), _syscalls(0), _cycles(0), _runtime(0), _cpu(0) {
		}

		std::string procName() const {
			return _procName;
		}
		tid_type tid() const {
			return _tid;
		}
		tid_type pid() const {
			return _pid;
		}
		state_type state() const {
			return _state;
		}
		Flags flags() const {
			return (Flags)_flags;
		}
		prio_type prio() const {
			return _prio;
		}
		size_type stackPages() const {
			return _stackPages;
		}
		size_type schedCount() const {
			return _schedCount;
		}
		size_type syscalls() const {
			return _syscalls;
		}
		cycle_type cycles() const {
			return _cycles;
		}
		time_type runtime() const {
			return _runtime;
		}
		cpu_type cpu() const {
			return _cpu;
		}

	private:
		tid_type _tid;
		pid_type _pid;
		std::string _procName;
		state_type _state;
		flags_type _flags;
		prio_type _prio;
		size_type _stackPages;
		size_type _schedCount;
		size_type _syscalls;
		cycle_type _cycles;
		time_type _runtime;
		cpu_type _cpu;
	};

	std::istream& operator >>(std::istream& is,thread& t);
	std::ostream& operator <<(std::ostream& os,const thread& t);
}
