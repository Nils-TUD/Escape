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

#pragma once

#include <gui/layout/flowlayout.h>
#include <gui/button.h>
#include <gui/panel.h>
#include <vector>

#include "../model/link.h"
#include "filelist.h"

class Favorites : public gui::Panel {
public:
	typedef std::vector<Link> favlist_type;

	explicit Favorites(std::shared_ptr<FileList> filelist,const favlist_type &favs)
			: Panel(gui::make_layout<gui::FlowLayout>(
					gui::FRONT,true,gui::VERTICAL,4)),
			  _filelist(filelist) {
		for(auto it = favs.begin(); it != favs.end(); ++it) {
			std::shared_ptr<gui::Button> b = gui::make_control<gui::Button>(it->getTitle());
			b->clicked().subscribe(gui::bind1_mem_recv(*it,this,&Favorites::onButtonClick));
			add(b);
		}
	}

private:
	void onButtonClick(const Link &fav,gui::UIElement&) const {
		_filelist->loadDir(fav.getPath());
	}

	std::shared_ptr<FileList> _filelist;
};
