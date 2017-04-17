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

#include <sys/common.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "timeouts.h"

#define print(...)

int Timeouts::_nextId;
std::list<Timeouts::Entry> Timeouts::_list;
extern std::mutex mutex;

void Timeouts::program(int id,callback_type *cb,uint msecs) {
	// first cancel the old one
	cancel(id);

	// insert new timeout, sorted in ascending order
	uint64_t ts = rdtsc() + timetotsc(msecs * 1000);
	auto it = _list.begin();
	for(; it != _list.end(); ++it) {
		if(it->timestamp > ts)
			break;
	}
	print("Inserting timeout %d @ %Luus",id,tsctotime(ts));
	if(it == _list.begin())
		kill(getpid(),SIGUSR2);
	_list.insert(it,Entry(id,cb,ts));
}

void Timeouts::cancel(int id) {
	for(auto it = _list.begin(); it != _list.end(); ++it) {
		if(it->id == id) {
			delete it->cb;
			print("Removing timeout %d",id);
			if(it == _list.begin())
				kill(getpid(),SIGUSR2);
			_list.erase(it);
			break;
		}
	}
}

static void sighdl(int) {
	signal(SIGUSR2,sighdl);
}

int Timeouts::thread(void*) {
	if(signal(SIGUSR2,sighdl) == SIG_ERR)
		error("Unable to set signal handler");

	while(1) {
		uint64_t next = std::numeric_limits<uint64_t>::max();
		if(_list.size() > 0)
			next = _list.front().timestamp - rdtsc();
		print("Sleeping for %Luus @ %Luus",tsctotime(next),tsctotime(rdtsc()));
		usleep(tsctotime(next));
		print("Waked up @ %Luus",tsctotime(rdtsc()));

		// it's sorted
		std::lock_guard<std::mutex> guard(mutex);
		uint64_t now = rdtsc();
		while(_list.size() > 0 && _list.front().timestamp <= now) {
			auto it = _list.begin();
			callback_type *cb = it->cb;
			print("Triggering timeout %d @ %Luus",it->id,tsctotime(rdtsc()));
			_list.erase(it);

			(*cb)();

			delete cb;
		}
	}
	return 0;
}
