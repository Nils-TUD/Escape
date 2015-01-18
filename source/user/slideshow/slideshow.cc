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

#include <gui/layout/borderlayout.h>
#include <gui/application.h>
#include <gui/image.h>
#include <gui/imagebutton.h>
#include <gui/window.h>
#include <sys/common.h>
#include <algorithm>
#include <vector>

using namespace gui;
using namespace std;

static size_t slide;
static vector<const char*> imgs;
static shared_ptr<Panel> root;
static shared_ptr<ImageButton> curimg;

static void showSlide() {
	const char *img = imgs[slide];
	if(curimg)
		root->remove(curimg,BorderLayout::CENTER);
	curimg = make_control<ImageButton>(Image::loadImage(img),false);
	root->add(curimg,BorderLayout::CENTER);

	root->layout();
	root->repaint();
}

static void onKeyReleased(UIElement &,const KeyEvent &e) {
	size_t oldslide = slide;
	switch(e.getKeyCode()) {
		case VK_UP:
		case VK_LEFT:
			if(slide > 0)
				slide--;
			break;
		case VK_DOWN:
		case VK_RIGHT:
			if(slide < imgs.size() - 1)
				slide++;
			break;
		case VK_HOME:
			slide = 0;
			break;
		case VK_END:
			slide = imgs.size() - 1;
			break;
	}
	if(oldslide != slide)
		showSlide();
}

int main(int argc,char **argv) {
	if(argc < 2)
		error("Usage: %s <image>...",argv[0]);

	for(int i = 1; i < argc; ++i)
		imgs.push_back(argv[i]);
	sort(imgs.begin(),imgs.end());
	slide = 0;

	Application *app = Application::create();
	shared_ptr<Window> w = make_control<Window>("Slideshow",Pos(0,0),app->getScreenSize());
	root = w->getRootPanel();
	root->setLayout(make_layout<BorderLayout>());
	w->keyReleased().subscribe(func_recv(onKeyReleased));
	showSlide();
	w->show();
	app->addWindow(w);

	return app->run();
}
