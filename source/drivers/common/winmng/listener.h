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

#include <esc/proto/winmng.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <mutex>
#include <vector>

class Listener {
public:
	typedef esc::WinMngEvents::Event::Type ev_type;

private:
	struct WinListener {
		explicit WinListener() : client(), type() {
		}
		explicit WinListener(int client,ev_type type) : client(client), type(type) {
		}

		int client;
		ev_type type;
	};

	typedef std::vector<WinListener>::iterator win_iter;

	explicit Listener(int id) : _drvId(id), _mutex(), _list() {
	}

public:
	static void create(int id) {
		_inst = new Listener(id);
	}
	static Listener &get() {
		return *_inst;
	}

	bool add(int client,ev_type type);
	void notify(const esc::WinMngEvents::Event *ev);
	void remove(int client,ev_type type);
	void removeAll(int client);

private:
	int _drvId;
	std::mutex _mutex;
	std::vector<WinListener> _list;
	static Listener *_inst;
};
