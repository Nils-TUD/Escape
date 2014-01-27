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

#include <esc/common.h>
#include <string>
#include <vector>

class Process {
public:
	Process(int id = 0,bool killable = true)
		: _pid(id), _alive(), _dead(), _killable(killable), _name(), _args() {
	}
	Process(const std::string& procName)
		: _pid(), _alive(), _dead(), _killable(true), _name(procName), _args() {
	}
	virtual ~Process() {
	}

	bool isAlive() const {
		return _alive;
	}
	void setAlive() {
		_alive = true;
	}
	bool isDead() const {
		return _dead;
	}
	void setDead() {
		_dead = true;
	}

	virtual bool isKillable() const {
		return _killable;
	}
	int pid() const {
		return _pid;
	}
	const std::string& name() const {
		return _name;
	}
	const std::vector<std::string>& args() const {
		return _args;
	}

	virtual void load() {
		// do nothing by default
	}

	friend bool operator<(Process &p1,Process &p2);

protected:
	int _pid;
	bool _alive;
	bool _dead;
	bool _killable;
	std::string _name;
	std::vector<std::string> _args;
};
