/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <esc/common.h>
#include <gui/image/bitmapimage.h>
#include <gui/layout/borderlayout.h>
#include <gui/layout/flowlayout.h>
#include <gui/layout/iconlayout.h>
#include <gui/application.h>
#include <gui/window.h>
#include <gui/button.h>
#include <gui/editable.h>
#include <gui/combobox.h>
#include <gui/checkbox.h>
#include <gui/progressbar.h>
#include <gui/scrollpane.h>
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>

using namespace gui;
using namespace std;

static void addWindow(Application *app,shared_ptr<Window> win);
static shared_ptr<Window> win0(void);
static shared_ptr<Window> win01(void);
static shared_ptr<Window> win1(void);
static shared_ptr<Window> win2(void);
static shared_ptr<Window> win3(void);
static shared_ptr<Window> win4(void);
static shared_ptr<Window> win5(void);
static shared_ptr<Window> win6(void);
static shared_ptr<Window> win7(void);
static int pbThread(void *arg);

static volatile bool run = true;

int main() {
	Application *app = Application::create();
	addWindow(app,win0());
	addWindow(app,win01());
	addWindow(app,win1());
	addWindow(app,win2());
	addWindow(app,win3());
	addWindow(app,win4());
	addWindow(app,win5());
	addWindow(app,win6());
	addWindow(app,win7());
	cout.flush();
	int res = app->run();
	run = false;
	join(0);
	return res;
}

static void addWindow(Application *app,shared_ptr<Window> win) {
	app->addWindow(win);
}

static shared_ptr<Window> win0(void) {
	shared_ptr<Window> w = make_control<Window>("Window 0",Pos(500,200));
	shared_ptr<Panel> root = w->getRootPanel();
	root->setLayout(make_layout<BorderLayout>());
	shared_ptr<Panel> p = make_control<Panel>(make_layout<BorderLayout>());
	shared_ptr<ScrollPane> sp = make_control<ScrollPane>(p);
	root->add(sp,BorderLayout::CENTER);
	p->add(make_control<ScrollPane>(
			make_control<ProgressBar>("Progress1...",Pos(0,0),Size(150,0))),BorderLayout::CENTER);
	p->add(make_control<ScrollPane>(
			make_control<ProgressBar>("Progress2...",Pos(0,0),Size(100,0))),BorderLayout::NORTH);
	p->add(make_control<ScrollPane>(
			make_control<ProgressBar>("Progress3...",Pos(0,0),Size(50,0))),BorderLayout::SOUTH);
	p->add(make_control<ScrollPane>(
			make_control<ProgressBar>("Progress4...")),BorderLayout::WEST);
	//if(startthread(pbThread,pb) < 0)
	//	cerr << "[GUITEST] Unable to start thread" << endl;
	w->show(true);
	return w;
}

static shared_ptr<Window> win01(void) {
	shared_ptr<Window> w = make_control<Window>("Window 01",Pos(600,250));
	shared_ptr<Panel> root = w->getRootPanel();
	root->setLayout(make_layout<BorderLayout>());
	shared_ptr<Panel> p = make_control<Panel>(make_layout<BorderLayout>());
	shared_ptr<ScrollPane> sp = make_control<ScrollPane>(p);
	root->add(sp,BorderLayout::CENTER);
	p->add(make_control<ScrollPane>(
			make_control<ProgressBar>("Progress1...",Pos(0,0),Size(150,0))),BorderLayout::CENTER);
	w->show(true);
	return w;
}

static shared_ptr<Window> win1(void) {
	shared_ptr<Window> w = make_control<Window>("Window 1",Pos(100,100));
	shared_ptr<Panel> root = w->getRootPanel();
	root->getTheme().setPadding(0);
	root->setLayout(make_layout<BorderLayout>());
	shared_ptr<Panel> p = make_control<Panel>(make_layout<BorderLayout>());
	shared_ptr<ScrollPane> sp = make_control<ScrollPane>(p);
	root->add(sp,BorderLayout::CENTER);

	shared_ptr<Button> b = make_control<Button>("Click me!!");
	p->add(b,BorderLayout::WEST);
	shared_ptr<Editable> e = make_control<Editable>();
	p->add(e,BorderLayout::EAST);
	shared_ptr<ComboBox> cb = make_control<ComboBox>();
	cb->addItem("Test item");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	p->add(cb,BorderLayout::NORTH);
	shared_ptr<Checkbox> check = make_control<Checkbox>("My Checkbox");
	p->add(check,BorderLayout::SOUTH);
	shared_ptr<ProgressBar> pb = make_control<ProgressBar>("Progress...",Pos(0,0),Size(150,0));
	shared_ptr<ScrollPane> sp2 = make_control<ScrollPane>(pb);
	p->add(sp2,BorderLayout::CENTER);
	//p->add(new ProgressBar("Progress..."),BorderLayout::CENTER);

	w->appendTabCtrl(*b);
	w->appendTabCtrl(*e);
	w->appendTabCtrl(*check);
	w->show(true);
	//if(startthread(pbThread,pb) < 0)
	//	cerr << "[GUITEST] Unable to start thread" << endl;
	return w;
}

static shared_ptr<Window> win2(void) {
	shared_ptr<Window> w = make_control<Window>("Window 2",Pos(450,150),Size(400,100));
	shared_ptr<Panel> root = w->getRootPanel();
	root->setLayout(make_layout<FlowLayout>(FlowLayout::FRONT,FlowLayout::HORIZONTAL,5));

	shared_ptr<Button> b = make_control<Button>("Try it!");
	root->add(b);
	shared_ptr<Editable> e = make_control<Editable>();
	root->add(e);
	shared_ptr<ComboBox> cb = make_control<ComboBox>();
	cb->addItem("Huhu!");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	root->add(cb);
	shared_ptr<Checkbox> check = make_control<Checkbox>("My Checkbox");
	root->add(check);
	shared_ptr<ProgressBar> pb = make_control<ProgressBar>("Running");
	root->add(pb);

	w->appendTabCtrl(*b);
	w->appendTabCtrl(*e);
	w->appendTabCtrl(*check);
	w->show();
	return w;
}

static shared_ptr<Window> win3(void) {
	shared_ptr<Window> w = make_control<Window>("Window 3",Pos(450,350),Size(400,100));
	shared_ptr<Panel> root = w->getRootPanel();
	root->setLayout(make_layout<FlowLayout>(FlowLayout::CENTER,FlowLayout::HORIZONTAL,1));

	shared_ptr<Button> b = make_control<Button>("Try it!");
	root->add(b);
	shared_ptr<Editable> e = make_control<Editable>();
	root->add(e);
	shared_ptr<ComboBox> cb = make_control<ComboBox>();
	cb->addItem("Huhu!");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	root->add(cb);
	shared_ptr<Checkbox> check = make_control<Checkbox>("My Checkbox");
	root->add(check);
	shared_ptr<ProgressBar> pb = make_control<ProgressBar>("Running");
	root->add(pb);

	w->appendTabCtrl(*b);
	w->appendTabCtrl(*e);
	w->appendTabCtrl(*check);
	w->show();
	return w;
}

static shared_ptr<Window> win4(void) {
	shared_ptr<Window> w = make_control<Window>("Window 4",Pos(150,350),Size(400,100));
	shared_ptr<Panel> root = w->getRootPanel();
	root->getTheme().setPadding(0);
	root->setLayout(make_layout<BorderLayout>());
	shared_ptr<Panel> p = make_control<Panel>(
			make_layout<FlowLayout>(FlowLayout::BACK,FlowLayout::VERTICAL,10));
	shared_ptr<ScrollPane> sp = make_control<ScrollPane>(p);
	root->add(sp,BorderLayout::CENTER);

	shared_ptr<Button> b = make_control<Button>("Try it!");
	p->add(b);
	shared_ptr<Editable> e = make_control<Editable>();
	p->add(e);
	shared_ptr<ComboBox> cb = make_control<ComboBox>();
	cb->addItem("Huhu!");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	p->add(cb);
	shared_ptr<Checkbox> check = make_control<Checkbox>("My Checkbox");
	p->add(check);
	shared_ptr<ProgressBar> pb = make_control<ProgressBar>("Running");
	p->add(pb);

	w->appendTabCtrl(*b);
	w->appendTabCtrl(*e);
	w->appendTabCtrl(*check);
	//if(startthread(pbThread,pb.get()) < 0)
	//	cerr << "[GUITEST] Unable to start thread" << endl;
	w->show();
	return w;
}

static shared_ptr<Window> win5(void) {
	shared_ptr<Window> w = make_control<Window>("Window 5",Pos(250,450),Size(200,300));
	shared_ptr<Panel> root = w->getRootPanel();
	root->getTheme().setPadding(0);
	root->setLayout(make_layout<BorderLayout>());
	shared_ptr<Panel> p = make_control<Panel>(make_layout<BorderLayout>(10));
	shared_ptr<ScrollPane> sp = make_control<ScrollPane>(p);
	root->add(sp,BorderLayout::CENTER);

	shared_ptr<Button> b = make_control<Button>("Try it!");
	p->add(b,BorderLayout::NORTH);
	shared_ptr<Editable> e = make_control<Editable>();
	p->add(e,BorderLayout::CENTER);
	shared_ptr<ComboBox> cb = make_control<ComboBox>();
	cb->addItem("Huhu!");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	p->add(cb,BorderLayout::SOUTH);

	w->appendTabCtrl(*b);
	w->appendTabCtrl(*e);
	w->show();
	return w;
}

static shared_ptr<Window> win6(void) {
	shared_ptr<Window> win = make_control<Window>("Window 6",Pos(200,300));
	shared_ptr<Panel> root = win->getRootPanel();
	root->getTheme().setPadding(2);
	root->setLayout(make_layout<BorderLayout>(2));

	shared_ptr<Panel> btns = make_control<Panel>(make_layout<IconLayout>(IconLayout::HORIZONTAL));
	shared_ptr<ScrollPane> sp = make_control<ScrollPane>(btns);
	root->add(sp,BorderLayout::CENTER);

	for(size_t i = 0; i < 23; ++i) {
		char name[12];
		itoa(name,sizeof(name),i);
		shared_ptr<Button> b = make_control<Button>(name,Pos(0,0),Size(40,40));
		btns->add(b);
	}
	win->show(true);
	return win;
}

static shared_ptr<Window> win7(void) {
	shared_ptr<Window> win = make_control<Window>("Window 7",Pos(400,300));
	shared_ptr<Panel> root = win->getRootPanel();
	root->getTheme().setPadding(2);
	root->setLayout(make_layout<BorderLayout>(2));

	shared_ptr<Panel> btns = make_control<Panel>(make_layout<IconLayout>(IconLayout::VERTICAL));
	shared_ptr<ScrollPane> sp = make_control<ScrollPane>(btns);
	root->add(sp,BorderLayout::CENTER);

	srand(time(nullptr));
	for(size_t i = 0; i < 29; ++i) {
		char name[12];
		itoa(name,sizeof(name),i);
		btns->add(make_control<Button>(name,Pos(0,0),Size(30 + rand() % 10,30 + rand() % 10)));
	}
	win->show(true);
	return win;
}

static int pbThread(void *arg) {
	ProgressBar *pb = (ProgressBar*)arg;
	bool forward = true;
	while(run) {
		if(pb != nullptr) {
			Application::getInstance()->executeLater(make_lambda([pb,&forward] {
				size_t pos = pb->getPosition();
				if(forward) {
					if(pos < 100)
						pb->setPosition(pos + 1);
					else
						forward = false;
				}
				else {
					if(pos > 0)
						pb->setPosition(pos - 1);
					else
						forward = true;
				}
				pb->repaint();
			}));
		}
		sleep(10);
	}
	return 0;
}
