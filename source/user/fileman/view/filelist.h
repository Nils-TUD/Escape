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

#include <esc/stream/std.h>
#include <esc/file.h>
#include <gui/layout/flowlayout.h>
#include <gui/layout/iconlayout.h>
#include <gui/imagebutton.h>
#include <gui/label.h>
#include <gui/panel.h>
#include <gui/scrollpane.h>
#include <dirent.h>
#include <list>

#include "pathbar.h"

class FileList : public gui::Panel {
	class FileObject : public gui::Panel {
	public:
		explicit FileObject(FileList &list,const esc::file &file)
			: Panel(gui::make_layout<gui::FlowLayout>(
					gui::CENTER,true,gui::VERTICAL,4)), _list(list) {
			using namespace std;
			using namespace gui;
			string path = file.is_dir() ? "/etc/folder.png" : "/etc/file.png";
			shared_ptr<ImageButton> img = make_control<ImageButton>(Image::loadImage(path),false);
			shared_ptr<Label> lbl = make_control<Label>(file.name());
			if(file.is_dir())
				img->clicked().subscribe(bind1_mem_recv(file,this,&FileObject::onClick));
			add(img);
			add(lbl);
		}

		void onClick(const esc::file &file,UIElement&) {
			gui::Application::getInstance()->executeLater(
					std::make_bind1_memfun(file,this,&FileObject::loadDir));
		}

	private:
		void loadDir(const esc::file &file) {
			_list.loadDir(file.path());
		}

		FileList &_list;
	};

public:
	explicit FileList(std::shared_ptr<PathBar> pathbar)
		: Panel(gui::make_layout<gui::IconLayout>(gui::HORIZONTAL,4)),
		  _pathbar(pathbar), _files() {
	}

	void loadDir(const std::string &path) {
		using namespace std;
		list<esc::file> files;
		try {
			_pathbar->setPath(this,path);
			vector<struct dirent> entries = esc::file(path).list_files(false);
			for(auto it = entries.begin(); it != entries.end(); ++it)
				files.push_back(esc::file(path,it->d_name));
			files.sort([] (const esc::file &a,const esc::file &b) {
				if(a.is_dir() == b.is_dir())
					return a.name() < b.name();
				return a.is_dir();
			});
			setList(files);
		}
		catch(const esc::default_error& e) {
			errmsg(e.what());
		}
	}

	void setList(const std::list<esc::file> &files) {
		removeAll();
		_files = files;
		for(auto it = _files.begin(); it != _files.end(); ++it)
			add(gui::make_control<FileObject>(*this,*it));
		layout();
		static_cast<gui::ScrollPane*>(getParent())->scrollToTop(false);
		repaint();
	}

private:
	std::shared_ptr<PathBar> _pathbar;
	std::list<esc::file> _files;
};
