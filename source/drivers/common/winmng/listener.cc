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
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/thread.h>
#include <mutex>
#include <stdlib.h>
#include <vector>

#include "listener.h"

Listener *Listener::_inst;

bool Listener::add(int client,ev_type type) {
	if(type != ev_type::TYPE_CREATED && type != ev_type::TYPE_DESTROYED &&
		type != ev_type::TYPE_ACTIVE)
		return false;

	std::lock_guard<std::mutex> guard(_mutex);
	_list.push_back(WinListener(client,type));
	return true;
}

void Listener::notify(const esc::WinMngEvents::Event *ev) {
	std::lock_guard<std::mutex> guard(_mutex);
	for(auto l = _list.begin(); l != _list.end(); ++l) {
		if(l->type == ev->type)
			send(l->client,MSG_WIN_EVENT,ev,sizeof(*ev));
	}
}

void Listener::remove(int client,ev_type type) {
	std::lock_guard<std::mutex> guard(_mutex);
	for(auto l = _list.begin(); l != _list.end(); ++l) {
		if(l->client == client && l->type == type) {
			_list.erase(l);
			break;
		}
	}
}

void Listener::removeAll(int client) {
	std::lock_guard<std::mutex> guard(_mutex);
	while(!_list.empty()) {
		win_iter it = std::find_if(_list.begin(),_list.end(),[client] (const WinListener &l) {
			return l.client == client;
		});
		if(it == _list.end())
			break;
		_list.erase(it);
	}
}
