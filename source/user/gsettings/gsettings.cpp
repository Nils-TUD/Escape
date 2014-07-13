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

#include <sys/common.h>
#include <gui/layout/gridlayout.h>
#include <gui/layout/flowlayout.h>
#include <gui/application.h>
#include <gui/window.h>
#include <gui/combobox.h>
#include <sstream>

using namespace std;
using namespace gui;

static shared_ptr<ComboBox> combo;
static vector<esc::Screen::Mode> modes;

static void onCancel(UIElement &) {
	Application::getInstance()->exit();
}

static void onApply(UIElement &) {
	ssize_t sel = combo->getSelectedIndex();
	if(sel >= 0) {
		try {
			Application::getInstance()->setMode(modes[sel]);
		}
		catch(const std::exception &e) {
			printe("%s",e.what());
		}
	}
}

int main() {
	Application *app = Application::create();
	shared_ptr<Window> win = make_control<Window>("Settings",Pos(200,200));
	shared_ptr<Panel> root = win->getRootPanel();
	root->setLayout(make_layout<BorderLayout>());

	shared_ptr<Panel> grid = make_control<Panel>(make_layout<GridLayout>(2,1,5));
	grid->add(make_control<Label>("Screenmode:"),GridPos(0,0));

	combo = make_control<ComboBox>();
	const esc::Screen::Mode *curmode = app->getMode();
	modes = app->getModes();
	for(auto it = modes.begin(); it != modes.end(); ++it) {
		ostringstream os;
		os << it->width << "x" << it->height << ", " << it->bitsPerPixel << "bpp";
		combo->addItem(os.str());
		if(curmode->id == it->id)
			combo->setSelectedIndex(it - modes.begin());
	}
	grid->add(combo,GridPos(1,0));

	root->add(grid,BorderLayout::CENTER);

	shared_ptr<Panel> btns = make_control<Panel>(make_layout<FlowLayout>(BACK,true,HORIZONTAL,5));
	shared_ptr<Button> cancel = make_control<Button>("Cancel");
	cancel->clicked().subscribe(func_recv(onCancel));
	btns->add(cancel);
	shared_ptr<Button> apply = make_control<Button>("Apply");
	apply->clicked().subscribe(func_recv(onApply));
	btns->add(apply);
	root->add(btns,BorderLayout::SOUTH);

	win->show(true);
	app->addWindow(win);
	return app->run();
}
