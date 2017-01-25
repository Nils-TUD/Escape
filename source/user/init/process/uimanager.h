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

#include <sys/common.h>
#include <signal.h>
#include <vector>

#include "process.h"

class UIManager {
public:
	explicit UIManager() : _uis() {
	}

	void add(const std::vector<Process*> &ui) {
		_uis.push_back(ui);
	}

	void died(pid_t pid) {
		for(auto ui = _uis.begin(); ui != _uis.end(); ++ui) {
			bool found = false;
			for(auto p = ui->begin(); p != ui->end(); ++p) {
				if((*p)->pid() == pid) {
					found = true;
					ui->erase(p);
					delete *p;
					break;
				}
			}

			if(found) {
				if(ui->size() == 0)
					_uis.erase(ui);
				else {
					for(auto p = ui->begin(); p != ui->end(); ++p)
						kill((*p)->pid(),SIGTERM);
				}
				break;
			}
		}
	}

private:
	std::vector<std::vector<Process*>> _uis;
};
