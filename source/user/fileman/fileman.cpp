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

#include <gui/window.h>
#include <gui/scrollpane.h>
#include <env.h>

#include "model/favorite.h"
#include "view/favorites.h"
#include "view/filelist.h"
#include "view/pathbar.h"

using namespace std;
using namespace gui;

int main() {
	vector<Favorite> favlist;
	favlist.push_back(Favorite("Root","/"));
	try {
		favlist.push_back(Favorite("Home",env::get("HOME")));
	}
	catch(const io_exception& e) {
		// TODO temporary
		favlist.push_back(Favorite("Home","/home/hrniels"));
		cerr << e.what() << endl;
	}

	Application *app = Application::create();
	shared_ptr<Window> w = make_control<Window>("File manager",Pos(100,100),Size(400,300));
	shared_ptr<Panel> root = w->getRootPanel();
	shared_ptr<PathBar> pathbar = make_control<PathBar>();
	shared_ptr<FileList> filelist = make_control<FileList>(pathbar);
	root->setLayout(make_layout<BorderLayout>());
	root->add(pathbar,BorderLayout::NORTH);
	root->add(make_control<Favorites>(filelist,favlist),BorderLayout::WEST);
	root->add(make_control<ScrollPane>(filelist),BorderLayout::CENTER);
	filelist->loadDir("/");
	w->show();
	app->addWindow(w);
	return app->run();
}
