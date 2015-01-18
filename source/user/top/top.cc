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

#include <esc/proto/vterm.h>
#include <esc/env.h>
#include <info/cpu.h>
#include <info/memusage.h>
#include <info/process.h>
#include <info/thread.h>
#include <sys/arch.h>
#include <sys/common.h>
#include <sys/conf.h>
#include <sys/esccodes.h>
#include <sys/keycodes.h>
#include <sys/sync.h>
#include <sys/thread.h>
#include <usergroup/user.h>
#include <assert.h>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <string>

#define PAGE_SCROLL		10

using namespace std;
using namespace info;

static volatile bool run = true;
static ssize_t yoffset;
static sUser *users;
static tUserSem displaySem;
static esc::VTerm vterm(esc::env::get("TERM").c_str());

template<typename T>
static void printBar(size_t barwidth,double ratio,const T& name) {
	uint filled = (uint)(barwidth * ratio);
	cout << "  " << setw(3) << left << name << "[";
	uint j = 0;
	for(; j < filled; ++j)
		cout << '|';
	for(; j < barwidth; ++j)
		cout << ' ';
}

static void printCPUBar(size_t barwidth,double ratio,cpu::id_type id) {
	printBar(barwidth,ratio,id);
	cout << right << setw(12) << setprecision(1) << 100 * ratio << "%]\n";
}

static void printMemBar(size_t barwidth,size_t used,size_t total,const char *name) {
	char stats[14];
	snprintf(stats,sizeof(stats),"%zu/%zuMB",used / (1024 * 1024),total / (1024 * 1024));
	double ratio = total == 0 ? 0 : used / (double)(total);
	printBar(barwidth,ratio,name);
	cout << right << setw(13) << stats << "]\n";
}

template<class T>
static void freevector(vector<T*> vec) {
	for(auto it = vec.begin(); it != vec.end(); ++it)
		delete *it;
}

static bool compareProcs(const process* a,const process* b) {
	return b->cycles() < a->cycles();
}

static void printSize(size_t size) {
	if(size >= 1024 * 1024 * 1024)
		cout << (size / (1024 * 1024 * 1024)) << "G";
	else if(size >= 1024 * 1024)
		cout << (size / (1024 * 1024)) << "M";
	else if(size >= 1024)
		cout << (size / 1024) << "K";
	else
		cout << size << "B";
}

static void display(void) {
	usemdown(&displaySem);
	try {
		size_t cpubarwidth;
		esc::Screen::Mode mode = vterm.getMode();
		cpubarwidth = mode.cols - SSTRLEN("  99 [10000/10000MB]  ");

		vector<cpu*> cpus = cpu::get_list();
		vector<process*> procs = process::get_list(false,0,true);
		std::sort(procs.begin(),procs.end(),compareProcs);
		memusage mem = memusage::get();

		cout << "\e[go;0;0]";
		for(uint y = 0; y < mode.rows; ++y) {
			for(uint x = 0; x < mode.cols; ++x)
				cout << " ";
		}
		cout << "\e[go;0;0]";

		for(auto it = cpus.begin(); it != cpus.end(); ++it) {
			double ratio = (*it)->usedCycles() / (double)((*it)->totalCycles());
			printCPUBar(cpubarwidth,ratio,(*it)->id());
		}
		freevector(cpus);

		printMemBar(cpubarwidth,mem.used(),mem.total(),"Mem");
		printMemBar(cpubarwidth,mem.swapUsed(),mem.swapTotal(),"Swp");
		cout << "\n";

		size_t wpid = 5;
		size_t wuser = 10;
		size_t wvirt = 5;
		size_t wphys = 5;
		size_t wshm = 5;
		size_t wcpu = 6;
		size_t wmem = 6;
		size_t wtime = 13;
		size_t cmdbegin = wpid + wuser + wvirt + wphys + wshm + wcpu + wmem + wtime;

		// print header
		cout << "\e[co;0;7]";
		cout << right << setw(wpid) << " PID" << left << setw(wuser) << " USER";
		cout << right << setw(wvirt) << " VIRT" << setw(wphys) << " PHYS" << setw(wshm) << " SHR";
		cout << setw(wcpu) << " CPU%" << setw(wmem) << " MEM%";
		cout << setw(wtime) << " TIME" << " Command";
		for(uint x = cmdbegin + SSTRLEN(" Command"); x < mode.cols; ++x)
			cout << ' ';
		cout << "\n\e[co]";

		// print threads
		size_t totalframes = 0;
		cpu::cycle_type totalcycles = 0;
		for(auto it = procs.begin(); it != procs.end(); ++it) {
			totalframes += (*it)->ownFrames();
			totalcycles += (*it)->cycles();
		}

		size_t i = 0;
		size_t y = cpus.size() + 4;
		size_t yoff = MIN(yoffset,MAX(0,(ssize_t)procs.size() - (ssize_t)(mode.rows - y)));
		auto it = procs.begin();
		advance(it,yoff);
		for(; y < mode.rows && it != procs.end(); ++it, ++i) {
			char time[12];
			const process &p = **it;
			sUser *user = user_getById(users,p.uid());

			cout << setw(wpid) << p.pid();
			cout << " " << left << setw(wuser - 1);
			if(user)
				cout.write(user->name,min(strlen(user->name),wuser - 1));
			else
				cout << p.uid();

			cout << right << setw(wvirt - 1);
			printSize(p.pages() * PAGE_SIZE);
			cout << setw(wphys - 1);
			printSize(p.ownFrames() * PAGE_SIZE);
			cout << setw(wshm - 1);
			printSize(p.sharedFrames() * PAGE_SIZE);

			cout << setw(wcpu) << setprecision(1) << (100.0 * (p.cycles() / (double)totalcycles));
			cout << setw(wmem) << setprecision(1) << (100.0 * (p.ownFrames() / (double)totalframes));

			process::cycle_type ms = p.runtime() / 1000;
			process::cycle_type mins = ms / (60 * 1000);
			ms %= 60 * 1000;
			process::cycle_type secs = ms / 1000;
			ms %= 1000;
			snprintf(time,sizeof(time),"%Lu:%02Lu.%03Lu",mins,secs,ms);
			cout << setw(wtime) << time;

			cout << " ";
			cout.write(p.command().c_str(),min(p.command().length(),mode.cols - cmdbegin - 1));
			if(++y >= mode.rows)
				break;
			cout << "\n";
		}
		freevector(procs);
		cout.flush();
	}
	catch(const exception &e) {
		cerr << "Update failed: " << e.what() << endl;
	}
	usemup(&displaySem);
}

static int refreshThread(void*) {
	while(run) {
		display();
		sleep(1000);
	}
	return 0;
}

int main(void) {
	vterm.setFlag(esc::VTerm::FL_NAVI,false);
	vterm.setFlag(esc::VTerm::FL_READLINE,false);
	vterm.backup();

	size_t usercount;
	users = user_parseFromFile(USERS_PATH,&usercount);

	if(usemcrt(&displaySem,1) < 0)
		error("Unable to create display lock");

	int tid = startthread(refreshThread,nullptr);
	if(tid < 0)
		error("startthread");

	/* open the "real" stdin, because stdin maybe redirected to something else */
	ifstream vt(esc::env::get("TERM").c_str());

	// read from vterm
	char c;
	while(run && (c = vt.get()) != EOF) {
		if(c == '\033') {
			istream::esc_type n1,n2,n3;
			istream::esc_type cmd = vt.getesc(n1,n2,n3);
			if(cmd != ESCC_KEYCODE || (n3 & STATE_BREAK))
				continue;
			switch(n2) {
				case VK_Q:
					run = false;
					break;
				case VK_UP:
					if(yoffset > 0)
						yoffset--;
					break;
				case VK_DOWN:
					yoffset++;
					break;
				case VK_PGUP:
					if(yoffset > PAGE_SCROLL)
						yoffset -= PAGE_SCROLL;
					else
						yoffset = 0;
					break;
				case VK_PGDOWN:
					yoffset += PAGE_SCROLL;
					break;
			}
			if(run)
				display();
		}
	}
	join(tid);

	vterm.setFlag(esc::VTerm::FL_NAVI,true);
	vterm.setFlag(esc::VTerm::FL_READLINE,true);
	vterm.restore();
	return EXIT_SUCCESS;
}
