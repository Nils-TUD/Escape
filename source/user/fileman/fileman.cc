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
	shared_ptr<Window> w = make_control<Window>("File manager",Pos(100,100),Size(450,300));
	shared_ptr<Panel> root = w->getRootPanel();
	shared_ptr<PathBar> pathbar = make_control<PathBar>();
	shared_ptr<FileList> filelist = make_control<FileList>(pathbar);
	root->setLayout(make_layout<BorderLayout>());
	root->add(make_control<Border>(pathbar,Border::BOTTOM),BorderLayout::NORTH);

	shared_ptr<SplitPanel> splitpan = make_control<SplitPanel>(VERTICAL);
	splitpan->getTheme().setPadding(0);
	splitpan->add(make_control<Favorites>(filelist,favlist));
	splitpan->add(make_control<ScrollPane>(filelist));
	root->add(splitpan,BorderLayout::CENTER);

	filelist->loadDir("/");
	w->show();
	app->addWindow(w);
	return app->run();
}
