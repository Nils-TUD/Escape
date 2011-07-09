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
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <iostream>

static int pbThread(void *arg);

static volatile bool run = true;

int main(void) {
	gui::Application *app = gui::Application::getInstance();
	gui::Window w1("Window 1",100,100,200,300);
	gui::Panel& root = w1.getRootPanel();
	gui::Button b("Click me!!",10,10,120,25);
	root.add(b);
	gui::Editable e(10,40,200,25);
	root.add(e);
	gui::ComboBox cb(10,80,100,25);
	cb.addItem("Test item");
	cb.addItem("Foo bar");
	cb.addItem("abc 123");
	root.add(cb);
	gui::Checkbox check("Meine Checkbox",10,120,200,20);
	root.add(check);
	gui::ProgressBar *pb = new gui::ProgressBar("Progress...",10,160,200,25);
	root.add(*pb);
	w1.appendTabCtrl(b);
	w1.appendTabCtrl(e);
	w1.appendTabCtrl(check);
	if(startThread(pbThread,pb) < 0)
		std::cerr << "[GUITEST] Unable to start thread" << std::endl;
	int res = app->run();
	run = false;
	join(0);
	return res;
}

static int pbThread(void *arg) {
	gui::ProgressBar *pb = (gui::ProgressBar*)arg;
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
