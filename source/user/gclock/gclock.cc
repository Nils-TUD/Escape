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
#include <gui/control.h>
#include <gui/window.h>
#include <sys/common.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

using namespace gui;

static const double PI = 3.14159265358979;

class ClockControl : public gui::Control {
public:
	virtual Size getPrefSize() const {
		return Size(200,200);
	}

	virtual void paint(Graphics &g) {
		time_t ts = time(NULL);
		struct tm *now = localtime(&ts);
		uint hour = now->tm_hour;
		uint min = now->tm_min;
		uint sec = now->tm_sec;

		gsize_t size = std::min(getSize().width,getSize().height) - 2;
		gsize_t radius = size / 2;
		gui::Pos center(getSize().width / 2,getSize().height / 2);

		g.setColor(getTheme().getColor(Theme::CTRL_FOREGROUND));
		g.fillCircle(center,radius);

		g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
		g.drawCircle(center,radius);
		g.drawCircle(center,radius - 4);
		g.fillCircle(center,4);

		drawHand(g,center,(hour >= 12 ? hour - 12 : hour) / 12.,size * 0.30);
		drawHand(g,center,min / 60.,size * 0.35);
		drawHand(g,center,sec / 60.,size * 0.40);

		double length = size / 2 - 20;
		for(int i = 1; i <= 12; ++i) {
			esc::OStringStream os;
			os << i;

			double value = i / 12.;
			gsize_t strwidth = g.getFont().getStringWidth(os.str().c_str());
			gsize_t strheight = g.getFont().getSize().height;
			gpos_t x = center.x + (gpos_t)(sin(2 * PI * value) * length);
			gpos_t y = center.y - (gpos_t)(cos(2 * PI * value) * length);
			g.drawString(x - strwidth / 2,y - strheight / 2,os.str(),0,os.str().length());
		}
	}

	void drawHand(Graphics &g,const Pos &center,double value,double length) {
		gpos_t x = center.x + (gpos_t)(sin(2 * PI * value) * length);
		gpos_t y = center.y - (gpos_t)(cos(2 * PI * value) * length);
		g.drawLine(center.x,center.y,x,y);
	}
};

static std::shared_ptr<ClockControl> clockcon;
static volatile bool run = true;

static void refresh() {
	clockcon->makeDirty(true);
	clockcon->repaint();
}

static void sigterm(int) {
}

static int refreshThread(void*) {
	if(signal(SIGUSR2,sigterm) == SIG_ERR)
		error("Unable to set signal handler");

	uint64_t tscinterval = timetotsc(1000 * 1000);
	while(run) {
		uint64_t now = rdtsc();

		Application::getInstance()->executeLater(std::make_fun(refresh));

		uint64_t next = now + tscinterval;
		now = rdtsc();
		if(next > now)
			usleep(tsctotime(next - now));
	}
	return 0;
}

int main() {
	Application *app = Application::create();
	std::shared_ptr<Window> win = make_control<Window>("Clock",Pos(250,250));
	std::shared_ptr<Panel> root = win->getRootPanel();
	root->getTheme().setPadding(2);
	root->setLayout(make_layout<BorderLayout>(2));

	clockcon = make_control<ClockControl>();
	root->add(clockcon,BorderLayout::CENTER);

	win->show(true);
	app->addWindow(win);

	if(startthread(refreshThread,NULL) < 0)
		error("Unable to start refresh thread");

	int res = app->run();

	run = false;
	kill(getpid(),SIGUSR2);
	join(0);
	return res;
}
