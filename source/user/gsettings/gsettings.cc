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

#include <esc/file.h>
#include <gui/layout/flowlayout.h>
#include <gui/layout/gridlayout.h>
#include <gui/application.h>
#include <gui/combobox.h>
#include <gui/window.h>
#include <sys/common.h>
#include <sstream>

using namespace std;
using namespace gui;

static shared_ptr<ComboBox> modeCombo;
static shared_ptr<ComboBox> themeCombo;
static vector<esc::Screen::Mode> modes;

static void onCancel(UIElement &) {
	Application::getInstance()->exit();
}

static void onApply(UIElement &) {
	const std::string *theme = themeCombo->getSelectedItem();
	try {
		if(theme && *theme != Application::getInstance()->getTheme())
			Application::getInstance()->setTheme(*theme);

		ssize_t sel = modeCombo->getSelectedIndex();
		if(sel >= 0 && modes[sel].id != Application::getInstance()->getMode()->id)
			Application::getInstance()->setMode(modes[sel].id);
	}
	catch(const std::exception &e) {
		printe("%s",e.what());
	}
}

int main() {
	Application *app = Application::create();
	auto win = make_control<Window>("Settings",Pos(200,200));
	auto root = win->getRootPanel();
	root->setLayout(make_layout<BorderLayout>());

	auto grid = make_control<Panel>(make_layout<GridLayout>(2,2,5));
	grid->add(make_control<Label>("Screenmode:"),GridPos(0,0));

	modeCombo = make_control<ComboBox>();
	const esc::Screen::Mode *curmode = app->getMode();
	modes = app->getModes();
	for(auto it = modes.begin(); it != modes.end(); ++it) {
		ostringstream os;
		os << it->width << "x" << it->height << ", " << it->bitsPerPixel << "bpp";
		modeCombo->addItem(os.str());
		if(curmode->id == it->id)
			modeCombo->setSelectedIndex(it - modes.begin());
	}
	grid->add(modeCombo,GridPos(1,0));

	grid->add(make_control<Label>("Theme:"),GridPos(0,1));

	std::string curTheme = Application::getInstance()->getTheme();
	themeCombo = make_control<ComboBox>();
	vector<struct dirent> themes = esc::file("/etc/themes").list_files(false);
	size_t i = 0;
	for(auto theme : themes) {
		themeCombo->addItem(theme.d_name);
		if(curTheme == theme.d_name)
			themeCombo->setSelectedIndex(i);
		i++;
	}
	grid->add(themeCombo,GridPos(1,1));

	root->add(grid,BorderLayout::CENTER);

	auto btns = make_control<Panel>(make_layout<FlowLayout>(BACK,true,HORIZONTAL,5));
	auto cancel = make_control<Button>("Cancel");
	cancel->clicked().subscribe(func_recv(onCancel));
	btns->add(cancel);
	auto apply = make_control<Button>("Apply");
	apply->clicked().subscribe(func_recv(onApply));
	btns->add(apply);
	root->add(btns,BorderLayout::SOUTH);

	win->show(true);
	app->addWindow(win);
	return app->run();
}
