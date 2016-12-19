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

#include <esc/env.h>
#include <gui/border.h>
#include <gui/scrollpane.h>
#include <gui/splitpanel.h>
#include <gui/window.h>

#include "model/link.h"
#include "view/favorites.h"
#include "view/filelist.h"
#include "view/pathbar.h"

using namespace std;
using namespace gui;

static shared_ptr<FileList> filelist;

static void listButtonClick(UIElement &) {
	filelist->setMode(FileList::MODE_LIST);
}
static void iconButtonClick(UIElement &) {
	filelist->setMode(FileList::MODE_ICONS);
}
static void keyPressed(UIElement &,const KeyEvent &e) {
	switch(e.getKeyCode()) {
		case VK_ESC:
			filelist->loadParent();
			break;
		case VK_ENTER:
			filelist->select();
			break;
		case VK_HOME:
			filelist->selectionTop();
			break;
		case VK_UP:
			filelist->selectionUp();
			break;
		case VK_DOWN:
			filelist->selectionDown();
			break;
		case VK_PGUP:
			filelist->selectionPageUp();
			break;
		case VK_PGDOWN:
			filelist->selectionPageDown();
			break;
		case VK_END:
			filelist->selectionBottom();
			break;
	}
}

int main() {
	vector<Link> favlist;
	favlist.push_back(Link("Root","/"));
	try {
		favlist.push_back(Link("Home",esc::env::get("HOME")));
	}
	catch(const esc::default_error& e) {
		errmsg(e.what());
	}

	Application *app = Application::create();
	shared_ptr<Window> w = make_control<Window>("File manager",Pos(100,100),Size(560,360));
	shared_ptr<Panel> root = w->getRootPanel();
	shared_ptr<PathBar> pathbar = make_control<PathBar>();
	filelist = make_control<FileList>(pathbar);
	root->setLayout(make_layout<BorderLayout>());

	shared_ptr<Panel> north = make_control<Panel>(make_layout<BorderLayout>());
	north->add(make_control<Label>("Location:"),BorderLayout::WEST);
	north->add(pathbar,BorderLayout::CENTER);
	shared_ptr<Panel> buttons = make_control<Panel>(make_layout<FlowLayout>(CENTER));

	shared_ptr<Button> listButton = make_control<Button>("List");
	listButton->clicked().subscribe(func_recv(listButtonClick));
	buttons->add(listButton);

	shared_ptr<Button> iconsButton = make_control<Button>("Icons");
	iconsButton->clicked().subscribe(func_recv(iconButtonClick));
	buttons->add(iconsButton);

	north->add(buttons,BorderLayout::EAST);
	root->add(make_control<Border>(north,Border::BOTTOM),BorderLayout::NORTH);

	shared_ptr<SplitPanel> splitpan = make_control<SplitPanel>(VERTICAL);
	splitpan->getTheme().setPadding(0);
	splitpan->add(make_control<Favorites>(filelist,favlist));
	splitpan->add(make_control<ScrollPane>(filelist));
	root->add(splitpan,BorderLayout::CENTER);

	w->keyPressed().subscribe(func_recv(keyPressed));

	filelist->loadDir("/");
	w->show();
	app->addWindow(w);
	return app->run();
}
