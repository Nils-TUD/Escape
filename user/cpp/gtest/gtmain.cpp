/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/proc.h>
#include <esc/debug.h>
#include <messages.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <esc/gui/application.h>
#include <esc/gui/window.h>
#include <esc/gui/button.h>
#include <esc/gui/editable.h>
#include <esc/gui/combobox.h>
#include <esc/gui/progressbar.h>
#include <esc/gui/bitmapimage.h>
#include <esc/date.h>
#include <stdlib.h>

using namespace esc::gui;

ProgressBar *pb = NULL;
Window *w1 = NULL;

static int pbThread(int argc,char *argv[]);

class MyImgWindow : public Window {
public:
	MyImgWindow()
		: Window("Window 2",250,250,500,400), img1(BitmapImage("/test.bmp")),
		img2(BitmapImage("/bbc.bmp")) {
	}
	~MyImgWindow() {
	}

	void paint(Graphics &g) {
		Window::paint(g);
		img1.paint(g,30,30);
		img2.paint(g,100,30);
	}

private:
	BitmapImage img1;
	BitmapImage img2;
};

int main(void) {
	if(fork() == 0) {
		Application *app = Application::getInstance();
		w1 = new Window("Window 1",100,100,400,300);
		Button b("Click me!!",10,10,80,20);
		Editable e(10,40,200,20);
		w1->add(b);
		w1->add(e);
		ComboBox cb(10,80,100,20);
		cb.addItem("Test item");
		cb.addItem("Foo bar");
		cb.addItem("abc 123");
		w1->add(cb);
		pb = new ProgressBar("Progress...",10,120,200,20);
		w1->add(*pb);
		startThread(pbThread,NULL);
		return app->run();
	}

	if(fork() == 0) {
		//Application *app = Application::getInstance();
		//w1 = new MyImgWindow();
		//return app->run();
		return 0;
	}

	if(fork() == 0) {
		//Application *app = Application::getInstance();
		//w1 = new Window("Window 3",50,50,100,40);
		//return app->run();
		return 0;
	}
#if 1
	if(fork() == 0) {
		exec("/bin/guishell",NULL);
		exit(EXIT_FAILURE);
	}
#endif

	/*while(1) {
		sleep(1000);
		debug();
	}*/

	Application *app = Application::getInstance();
	w1 = new Window("Window 4",180,90,200,100);
	/*startThread(pbThread);*/
	return app->run();
}

static int pbThread(int argc,char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);
	/*while(1) {
		sleep(1000);
		if(fork() == 0)
			exec("/bin/ps",NULL);
	}*/
	/*s16 x = 10,y = 10;*/
	bool forward = true;
	while(1) {
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
			u32 pos = pb->getPosition();
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
