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

#include "../model/path.h"
#include "filelist.h"
#include "pathbar.h"

using namespace std;
using namespace gui;

void PathBar::setPath(FileList *list,const string &path) {
	_list = list;
	removeAll();
	Path parts(path);
	for(auto it = parts.begin(); it != parts.end(); ++it) {
		auto btn = make_control<Button>(it->getTitle());
		btn->clicked().subscribe(bind1_mem_recv(it->getPath(),this,&PathBar::onClick));
		add(btn);
	}
	layout();
	repaint();
}

void PathBar::onClick(string path,UIElement&) {
	_list->loadDir(path);
}
