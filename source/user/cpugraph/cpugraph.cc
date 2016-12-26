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

#include <gui/application.h>
#include <gui/layout/gridlayout.h>
#include <gui/window.h>
#include <info/cpu.h>
#include <sys/common.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/time.h>
#include <sstream>

using namespace std;
using namespace gui;

class CPUGraph : public Control {
public:
	explicit CPUGraph(int idx) : Control(), _idx(idx) {
	}

	virtual void paint(Graphics &g);

private:
	virtual Size getPrefSize() const {
		return Size(100,40);
	}

	int _idx;
};

static volatile bool run = true;
static vector<shared_ptr<CPUGraph>> graphs;
static vector<list<double>> cpus;

void CPUGraph::paint(Graphics &g) {
	g.setColor(Color(0xFF,0xFF,0xFF));
	g.fillRect(0,0,getSize().width,getSize().height);

	g.setColor(Color(0,0,0));
	list<double> &l = cpus[_idx];
	gpos_t x = 0;
	gsize_t height = getSize().height;
	for(auto it = l.begin(); it != l.end(); ++it, ++x)
		g.drawLine(x,height,x,height - (*it) * height);
}

static void refresh() {
	std::vector<info::cpu*> cpulist = info::cpu::get_list();
	int no = 0;
	for(auto it = cpulist.begin(); it != cpulist.end(); ++it, ++no) {
		gsize_t width = graphs[no]->getSize().width;
		list<double> &l = cpus[no];
		while(l.size() > width)
			l.pop_front();
		l.push_back((*it)->usedCycles() / (double)(*it)->totalCycles());

		graphs[no]->makeDirty(true);
		graphs[no]->repaint();
	}
}

static void sigusr1(int) {
	run = false;
}

static int refreshThread(void *) {
	if(signal(SIGUSR1,sigusr1) == SIG_ERR)
		error("Unable to set USR1 handler");

	uint64_t waittime = timetotsc(1000000);
	while(run) {
		uint64_t now = rdtsc();

		Application::getInstance()->executeLater(make_fun(refresh));

		usleep(tsctotime(waittime - (rdtsc() - now)));
	}
	return 0;
}

int main() {
	Application *app = Application::create();
	shared_ptr<Window> win = make_control<Window>("CPUGraph",Pos(300,300));
	shared_ptr<Panel> root = win->getRootPanel();
	root->setLayout(make_layout<BorderLayout>());

	std::vector<info::cpu*> cpulist = info::cpu::get_list();

	shared_ptr<Panel> grid = make_control<Panel>(
		make_layout<GridLayout>(1,cpulist.size()));

	int no = 0;
	for(auto it = cpulist.begin(); it != cpulist.end(); ++it, ++no) {
		graphs[no] = make_control<CPUGraph>(no);
		grid->add(graphs[no],GridPos(0,no));
	}
	root->add(grid,BorderLayout::CENTER);

	int tid;
	if((tid = startthread(refreshThread,NULL)) < 0)
		error("Unable to start thread");

	win->show(true);
	app->addWindow(win);
	int res = app->run();

	kill(getpid(),SIGUSR1);
	join(tid);
	return res;
}
