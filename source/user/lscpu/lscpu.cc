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

#include <esc/stream/std.h>
#include <info/cpu.h>
#include <sys/common.h>

#include "CPUInfo.h"

int main() {
	CPUInfo *info = CPUInfo::create();

	int i = 0;
	std::vector<info::cpu*> cpus = info::cpu::get_list();
	for(auto it = cpus.begin(); it != cpus.end(); ++it, ++i) {
		esc::sout << "CPU " << i << ":\n";
		info->print(esc::sout,**it);
		if(it + 1 != cpus.end())
			esc::sout << "\n";
	}
	return 0;
}
