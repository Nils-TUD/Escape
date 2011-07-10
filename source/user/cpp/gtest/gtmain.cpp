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
#include <gui/application.h>
#include <gui/window.h>
#include <gui/button.h>
#include <gui/editable.h>
#include <gui/combobox.h>
#include <gui/checkbox.h>
#include <gui/progressbar.h>
#include <gui/bitmapimage.h>
#include <gui/borderlayout.h>
#include <gui/flowlayout.h>
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <iostream>

using namespace gui;

static void win1(void);
static void win2(void);
static void win3(void);
static void win4(void);
static int pbThread(void *arg);

static volatile bool run = true;

int main(void) {
	Application *app = Application::getInstance();
	win1();
	win2();
	win3();
	win4();
	int res = app->run();
	run = false;
	join(0);
	return res;
}

static void win1(void) {
	Window *w1 = new Window("Window 1",100,100,300,200);
	Panel& root = w1->getRootPanel();
	root.setLayout(new BorderLayout());

	Button *b = new Button("Click me!!");
	root.add(*b,BorderLayout::WEST);
	Editable *e = new Editable();
	root.add(*e,BorderLayout::EAST);
	ComboBox *cb = new ComboBox();
	cb->addItem("Test item");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	root.add(*cb,BorderLayout::NORTH);
	Checkbox *check = new Checkbox("My Checkbox");
	root.add(*check,BorderLayout::SOUTH);
	ProgressBar *pb = new ProgressBar("Progress...");
	root.add(*pb,BorderLayout::CENTER);
	w1->appendTabCtrl(*b);
	w1->appendTabCtrl(*e);
	w1->appendTabCtrl(*check);
	if(startThread(pbThread,pb) < 0)
		std::cerr << "[GUITEST] Unable to start thread" << std::endl;
}

static void win2(void) {
	Window *w2 = new Window("Window 2",450,150,400,100);
	Panel& root = w2->getRootPanel();
	root.setLayout(new FlowLayout(FlowLayout::LEFT,5));

	Button *b = new Button("Try it!");
	root.add(*b);
	Editable *e = new Editable();
	root.add(*e);
	ComboBox *cb = new ComboBox();
	cb->addItem("Huhu!");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	root.add(*cb);
	Checkbox *check = new Checkbox("My Checkbox");
	root.add(*check);
	ProgressBar *pb = new ProgressBar("Running");
	root.add(*pb);
	w2->appendTabCtrl(*b);
	w2->appendTabCtrl(*e);
	w2->appendTabCtrl(*check);
}

static void win3(void) {
	Window *w3 = new Window("Window 3",450,350,400,100);
	Panel& root = w3->getRootPanel();
	root.setLayout(new FlowLayout(FlowLayout::CENTER,1));

	Button *b = new Button("Try it!");
	root.add(*b);
	Editable *e = new Editable();
	root.add(*e);
	ComboBox *cb = new ComboBox();
	cb->addItem("Huhu!");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	root.add(*cb);
	Checkbox *check = new Checkbox("My Checkbox");
	root.add(*check);
	ProgressBar *pb = new ProgressBar("Running");
	root.add(*pb);
	w3->appendTabCtrl(*b);
	w3->appendTabCtrl(*e);
	w3->appendTabCtrl(*check);
}

static void win4(void) {
	Window *w3 = new Window("Window 4",750,350,400,100);
	Panel& root = w3->getRootPanel();
	root.setLayout(new FlowLayout(FlowLayout::RIGHT,10));

	Button *b = new Button("Try it!");
	root.add(*b);
	Editable *e = new Editable();
	root.add(*e);
	ComboBox *cb = new ComboBox();
	cb->addItem("Huhu!");
	cb->addItem("Foo bar");
	cb->addItem("abc 123");
	root.add(*cb);
	Checkbox *check = new Checkbox("My Checkbox");
	root.add(*check);
	ProgressBar *pb = new ProgressBar("Running");
	root.add(*pb);
	w3->appendTabCtrl(*b);
	w3->appendTabCtrl(*e);
	w3->appendTabCtrl(*check);
}

static int pbThread(void *arg) {
	ProgressBar *pb = (ProgressBar*)arg;
	bool forward = true;
	while(run) {
		/*if(w1->getX() + w1->getWidth() >= Application::getInstance()->getScreenWidth() - 1)
			x = -10;
		else if(w1->getX() == 0)
			x = 10;
		if(w1->getY() + w1->getHeight() >= Application::getInstance()->getScreenHeight() - 1)
			y = -10;
		else if(w1->getY() == 0)
			y = 10;
		w1->move(x,y);*/
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
