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

static void win0(void);
static void win01(void);
static void win1(void);
static void win2(void);
static void win3(void);
static void win4(void);
static void win5(void);
static void win6(void);
static void win7(void);
static int pbThread(void *arg);

static volatile bool run = true;

int main(void) {
	Application *app = Application::getInstance();
	win0();
	win01();
	win1();
	win2();
	win3();
	win4();
	win5();
	win6();
	win7();
	int res = app->run();
	run = false;
	join(0);
	return res;
}

static void win0(void) {
	Window *w = new Window("Window 0",Pos(500,200));
	Panel& root = w->getRootPanel();
	root.setLayout(new BorderLayout());
	Panel *p = new Panel(new BorderLayout());
	ScrollPane *sp = new ScrollPane(p);
	root.add(sp,BorderLayout::CENTER);
	p->add(new ScrollPane(new ProgressBar("Progress1...",Pos(0,0),Size(150,0))),BorderLayout::CENTER);
	p->add(new ScrollPane(new ProgressBar("Progress2...",Pos(0,0),Size(100,0))),BorderLayout::NORTH);
	p->add(new ScrollPane(new ProgressBar("Progress3...",Pos(0,0),Size(50,0))),BorderLayout::SOUTH);
	p->add(new ScrollPane(new ProgressBar("Progress4...")),BorderLayout::WEST);
	//if(startthread(pbThread,pb) < 0)
	//	std::cerr << "[GUITEST] Unable to start thread" << std::endl;
	w->show(true);
}

static void win01(void) {
	Window *w = new Window("Window 01",Pos(600,250));
	Panel& root = w->getRootPanel();
	root.setLayout(new BorderLayout());
	Panel *p = new Panel(new BorderLayout());
	ScrollPane *sp = new ScrollPane(p);
	root.add(sp,BorderLayout::CENTER);
	p->add(new ScrollPane(new ProgressBar("Progress1...",Pos(0,0),Size(150,0))),BorderLayout::CENTER);
	w->show(true);
}

static void win1(void) {
	Window *w = new Window("Window 1",Pos(100,100));
	Panel& root = w->getRootPanel();
	root.getTheme().setPadding(0);
	root.setLayout(new BorderLayout());
	Panel *p = new Panel(new BorderLayout());
	ScrollPane *sp = new ScrollPane(p);
	root.add(sp,BorderLayout::CENTER);

	Button *b = new Button("Click me!!");
	p->add(b,BorderLayout::WEST);
	Editable *e = new Editable();
	p->add(e,BorderLayout::EAST);
	ComboBox *cb = new ComboBox();
	cb->addItem("Test item");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	p->add(cb,BorderLayout::NORTH);
	Checkbox *check = new Checkbox("My Checkbox");
	p->add(check,BorderLayout::SOUTH);
	ProgressBar *pb = new ProgressBar("Progress...",Pos(0,0),Size(150,0));
	ScrollPane *sp2 = new ScrollPane(pb);
	p->add(sp2,BorderLayout::CENTER);
	//p->add(new ProgressBar("Progress..."),BorderLayout::CENTER);

	w->appendTabCtrl(*b);
	w->appendTabCtrl(*e);
	w->appendTabCtrl(*check);
	w->show(true);
	//if(startthread(pbThread,pb) < 0)
	//	std::cerr << "[GUITEST] Unable to start thread" << std::endl;
}

static void win2(void) {
	Window *w = new Window("Window 2",Pos(450,150),Size(400,100));
	Panel& root = w->getRootPanel();
	root.setLayout(new FlowLayout(FlowLayout::LEFT,5));

	Button *b = new Button("Try it!");
	root.add(b);
	Editable *e = new Editable();
	root.add(e);
	ComboBox *cb = new ComboBox();
	cb->addItem("Huhu!");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	root.add(cb);
	Checkbox *check = new Checkbox("My Checkbox");
	root.add(check);
	ProgressBar *pb = new ProgressBar("Running");
	root.add(pb);

	w->appendTabCtrl(*b);
	w->appendTabCtrl(*e);
	w->appendTabCtrl(*check);
	w->show();
}

static void win3(void) {
	Window *w = new Window("Window 3",Pos(450,350),Size(400,100));
	Panel& root = w->getRootPanel();
	root.setLayout(new FlowLayout(FlowLayout::CENTER,1));

	Button *b = new Button("Try it!");
	root.add(b);
	Editable *e = new Editable();
	root.add(e);
	ComboBox *cb = new ComboBox();
	cb->addItem("Huhu!");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	root.add(cb);
	Checkbox *check = new Checkbox("My Checkbox");
	root.add(check);
	ProgressBar *pb = new ProgressBar("Running");
	root.add(pb);

	w->appendTabCtrl(*b);
	w->appendTabCtrl(*e);
	w->appendTabCtrl(*check);
	w->show();
}

static void win4(void) {
	Window *w = new Window("Window 4",Pos(150,350),Size(400,100));
	Panel& root = w->getRootPanel();
	root.getTheme().setPadding(0);
	root.setLayout(new BorderLayout());
	Panel *p = new Panel(new FlowLayout(FlowLayout::RIGHT,10));
	ScrollPane *sp = new ScrollPane(p);
	root.add(sp,BorderLayout::CENTER);

	Button *b = new Button("Try it!");
	p->add(b);
	Editable *e = new Editable();
	p->add(e);
	ComboBox *cb = new ComboBox();
	cb->addItem("Huhu!");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	p->add(cb);
	Checkbox *check = new Checkbox("My Checkbox");
	p->add(check);
	ProgressBar *pb = new ProgressBar("Running");
	p->add(pb);

	w->appendTabCtrl(*b);
	w->appendTabCtrl(*e);
	w->appendTabCtrl(*check);
	w->show();
}

static void win5(void) {
	Window *w = new Window("Window 5",Pos(250,450),Size(200,300));
	Panel& root = w->getRootPanel();
	root.getTheme().setPadding(0);
	root.setLayout(new BorderLayout());
	Panel *p = new Panel(new BorderLayout(10));
	ScrollPane *sp = new ScrollPane(p);
	root.add(sp,BorderLayout::CENTER);

	Button *b = new Button("Try it!");
	p->add(b,BorderLayout::NORTH);
	Editable *e = new Editable();
	p->add(e,BorderLayout::CENTER);
	ComboBox *cb = new ComboBox();
	cb->addItem("Huhu!");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	p->add(cb,BorderLayout::SOUTH);

	w->appendTabCtrl(*b);
	w->appendTabCtrl(*e);
	w->show();
}

static void win6(void) {
	Window *win = new Window("Window 6",Pos(200,300));
	Panel& root = win->getRootPanel();
	root.getTheme().setPadding(2);
	root.setLayout(new BorderLayout(2));

	Panel *btns = new Panel(new IconLayout(IconLayout::HORIZONTAL));
	ScrollPane *sp = new ScrollPane(btns);
	root.add(sp,BorderLayout::CENTER);

	for(size_t i = 0; i < 23; ++i) {
		char name[12];
		itoa(name,sizeof(name),i);
		Button *b = new Button(name,Pos(0,0),Size(40,40));
		btns->add(b);
	}
	win->show(true);
}

static void win7(void) {
	Window *win = new Window("Window 7",Pos(400,300));
	Panel& root = win->getRootPanel();
	root.getTheme().setPadding(2);
	root.setLayout(new BorderLayout(2));

	Panel *btns = new Panel(new IconLayout(IconLayout::VERTICAL));
	ScrollPane *sp = new ScrollPane(btns);
	root.add(sp,BorderLayout::CENTER);

	srand(time(NULL));
	for(size_t i = 0; i < 29; ++i) {
		char name[12];
		itoa(name,sizeof(name),i);
		Button *b = new Button(name,Pos(0,0),Size(30 + rand() % 10,30 + rand() % 10));
		btns->add(b);
	}
	win->show(true);
}

static int pbThread(void *arg) {
	ProgressBar *pb = (ProgressBar*)arg;
	bool forward = true;
	while(run) {
		/*if(w->getX() + w->getWidth() >= Application::getInstance()->getScreenWidth() - 1)
			x = -10;
		else if(w->getX() == 0)
			x = 10;
		if(w->getY() + w->getHeight() >= Application::getInstance()->getScreenHeight() - 1)
			y = -10;
		else if(w->getY() == 0)
			y = 10;
		w->move(x,y);*/
		if(pb != NULL) {
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
		}
		sleep(50);
	}
	return 0;
}
