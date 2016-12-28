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

#include "input.h"
#include "stack.h"
#include "winlist.h"

std::list<gwinid_t> Stack::stack;
std::list<gwinid_t>::iterator Stack::pos;
bool Stack::switched;

void Stack::add(gwinid_t id) {
	stack.push_back(id);
	start();
}

void Stack::remove(gwinid_t id) {
	auto sid = std::find(stack.begin(),stack.end(),id);
	assert(sid != stack.end());
	stack.erase(sid);
	start();
}

void Stack::activate(gwinid_t id) {
	remove(id);
	stack.push_front(id);
	start();
}

void Stack::start() {
	pos = stack.begin();
	switched = false;
}

void Stack::stop() {
	if(switched) {
		Window *win = WinList::get().getActive();
		if(!win)
			return;

		// move the active window to the top of the stack
		activate(win->id);
	}
}

void Stack::prev() {
	if(stack.empty())
		return;

	// to prev
	if(pos == stack.begin())
		pos = stack.end();
	--pos;
	// make it active
	WinList::get().setActive(WinList::get().get(*pos),true,false);
	switched = true;
}

void Stack::next() {
	if(stack.empty())
		return;

	// to next
	pos++;
	if(pos == stack.end())
		pos = stack.begin();
	// make it active
	WinList::get().setActive(WinList::get().get(*pos),true,false);
	switched = true;
}
