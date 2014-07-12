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
#include <sys/thread.h>

#include "timeouts.h"

uint Timeouts::_now;
int Timeouts::_nextId;
std::list<Timeouts::Entry> Timeouts::_list;
extern std::mutex mutex;

void Timeouts::program(int id,callback_type *cb,uint msecs) {
	// first cancel the old one
	cancel(id);

	// insert new timeout, sorted in ascending order
	uint ts = _now + msecs;
	auto it = _list.begin();
	for(; it != _list.end(); ++it) {
		if(it->timestamp > ts)
			break;
	}
	_list.insert(it,Entry(id,cb,ts));
}

void Timeouts::cancel(int id) {
	for(auto it = _list.begin(); it != _list.end(); ++it) {
		if(it->id == id) {
			delete it->cb;
			_list.erase(it);
			break;
		}
	}
}

int Timeouts::thread(void*) {
	while(1) {
		// TODO we shouldn't wake up all the time when there is no timeout to trigger
		sleep(100);
		_now += 100;

		// it's sorted
		std::lock_guard<std::mutex> guard(mutex);
		while(_list.size() > 0 && _list.front().timestamp <= _now) {
			auto it = _list.begin();
			callback_type *cb = it->cb;
			_list.erase(it);

			(*cb)();

			delete cb;
		}
	}
	return 0;
}
