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

#pragma once

#include <gui/layout/iconlayout.h>
#include <gui/layout/flowlayout.h>
#include <gui/panel.h>
#include <gui/imagebutton.h>
#include <gui/label.h>
#include <file.h>
#include <list>

class FileList : public gui::Panel {
	class FileObject : public gui::Panel {
	public:
		explicit FileObject(FileList &list,const std::file &file)
			: Panel(gui::make_layout<gui::FlowLayout>(
					gui::FlowLayout::CENTER,gui::FlowLayout::VERTICAL,4)), _list(list) {
			std::string path = file.is_dir() ? "/etc/folder.bmp" : "/etc/file.bmp";
			std::shared_ptr<gui::ImageButton> img = gui::make_control<gui::ImageButton>(
					gui::Image::loadImage(path),false);
			std::shared_ptr<gui::Label> lbl = gui::make_control<gui::Label>(file.name());
			if(file.is_dir())
				img->clicked().subscribe(gui::bind1_mem_recv(file,this,&FileObject::onClick));
			add(img);
			add(lbl);
		}

		void onClick(const std::file &file,UIElement&) {
			_list.loadDir(file.path());
		}

	private:
		FileList &_list;
	};

public:
	explicit FileList() : Panel(gui::make_layout<gui::IconLayout>(gui::IconLayout::HORIZONTAL,4)) {
	}

	void loadDir(const std::string &path) {
		std::list<std::file> files;
		std::vector<sDirEntry> entries = std::file(path).list_files(false);
		for(auto it = entries.begin(); it != entries.end(); ++it)
			files.push_back(std::file(path,it->name));
		files.sort([] (const std::file &a,const std::file &b) {
			if(a.is_dir() == b.is_dir())
				return a.name() < b.name();
			return a.is_dir();
		});
		setList(files);
	}
	void setList(const std::list<std::file> &files) {
		removeAll();
		_files = files;
		for(auto it = _files.begin(); it != _files.end(); ++it)
			add(gui::make_control<FileObject>(*this,*it));
		layout();
		repaint();
	}

private:
	std::list<std::file> _files;
};
