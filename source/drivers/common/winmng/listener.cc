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
#include <sys/thread.h>
#include <sys/io.h>
#include <stdlib.h>
#include <mutex>
#include <vector>

#include "listener.h"

struct WinListener {
	explicit WinListener() : client(), type() {
	}
	explicit WinListener(int client,ev_type type) : client(client), type(type) {
	}

	int client;
	ev_type type;
};

typedef std::vector<WinListener>::iterator win_iter;

static int drvId;
static std::mutex mutex;
static std::vector<WinListener> list;

void listener_init(int id) {
	drvId = id;
}

bool listener_add(int client,ev_type type) {
	if(type != ev_type::TYPE_CREATED && type != ev_type::TYPE_DESTROYED &&
		type != ev_type::TYPE_ACTIVE)
		return false;

	std::lock_guard<std::mutex> guard(mutex);
	list.push_back(WinListener(client,type));
	return true;
}

void listener_notify(const esc::WinMngEvents::Event *ev) {
	std::lock_guard<std::mutex> guard(mutex);
	for(auto l = list.begin(); l != list.end(); ++l) {
		if(l->type == ev->type)
			send(l->client,MSG_WIN_EVENT,ev,sizeof(*ev));
	}
}

void listener_remove(int client,ev_type type) {
	std::lock_guard<std::mutex> guard(mutex);
	for(auto l = list.begin(); l != list.end(); ++l) {
		if(l->client == client && l->type == type) {
			list.erase(l);
			break;
		}
	}
}

void listener_removeAll(int client) {
	std::lock_guard<std::mutex> guard(mutex);
	while(!list.empty()) {
		win_iter it = std::find_if(list.begin(),list.end(),[client] (const WinListener &l) {
			return l.client == client;
		});
		if(it == list.end())
			break;
		list.erase(it);
	}
}
