/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <esc/common.h>
#include <esc/driver/vterm.h>
#include <proc/process.h>
#include <proc/thread.h>
#include <fstream>
#include <iomanip>
#include <string>
#include <stdlib.h>

using namespace std;
using namespace proc;

static size_t getCPUCount() {
	size_t count = 0;
	ifstream f("/system/cpu");
	while(!f.eof()) {
		string line;
		f.getline(line,'\n');
		if(line.find("CPU") == 0)
			count++;
	}
	return count;
}

int main(int argc,char **argv) {
	sVTSize consSize;
	size_t cpus = getCPUCount();
	if(vterm_getSize(STDIN_FILENO,&consSize) < 0)
		error("Unable to determine screensize");
	vterm_setNavi(STDIN_FILENO,false);
	vterm_setReadline(STDIN_FILENO,false);

	size_t cpubarwidth = consSize.width / 2 - SSTRLEN("  99 [100.0%]  ");
	process::cycle_type *usedcycles = new process::cycle_type[cpus];
	process::cycle_type *idlecycles = new process::cycle_type[cpus];
	for(size_t i = 0; i < cpus; ++i)
		usedcycles[i] = idlecycles[i] = 0;

	vector<process*> procs = process::get_list();
	for(vector<process*>::const_iterator it = procs.begin(); it != procs.end(); ++it) {
		const vector<thread*> &threads = (*it)->threads();
		for(vector<thread*>::const_iterator tit = threads.begin(); tit != threads.end(); ++tit) {
			if((*tit)->flags() & thread::FL_IDLE)
				idlecycles[(*tit)->cpu()] += (*tit)->cycles();
			else
				usedcycles[(*tit)->cpu()] += (*tit)->cycles();
		}
	}

	for(size_t i = 0; i < cpus; ++i) {
		cout << usedcycles[i] << "," << idlecycles[i] << endl;
		double ratio = usedcycles[i] / (double)(usedcycles[i] + idlecycles[i]);
		uint filled = (uint)(cpubarwidth * ratio);
		cout << "  " << setw(3) << left << i << "[";
		uint j = 0;
		for(; j < filled; ++j)
			cout << '|';
		for(; j < cpubarwidth; ++j)
			cout << ' ';
		cout << right << setw(5) << setprecision(1) << 100 * ratio << "%]\n";
	}

	vterm_setNavi(STDIN_FILENO,true);
	vterm_setReadline(STDIN_FILENO,true);
	return EXIT_SUCCESS;
}
