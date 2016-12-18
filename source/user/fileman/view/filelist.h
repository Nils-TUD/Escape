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
#include <gui/layout/borderlayout.h>
#include <gui/layout/flowlayout.h>
#include <gui/layout/iconlayout.h>
#include <gui/layout/tablelayout.h>
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
		explicit FileObject(FileList &list,const esc::file &file) : Panel(), _list(list), _file(file) {
		}

		virtual void onClick() {
			if(_file.is_dir()) {
				gui::Application::getInstance()->executeLater(
						std::make_memfun(this,&FileObject::loadDir));
			}
		}

	private:
		void loadDir() {
			_list.loadDir(_file.path());
		}

		FileList &_list;
		esc::file _file;
	};

	class ListFileName : public FileObject {
	public:
		explicit ListFileName(FileList &list,const esc::file &file) : FileObject(list,file) {
			using namespace gui;
			setLayout(make_layout<BorderLayout>(4));

			std::string path = file.is_dir() ? "/etc/folder.png" : "/etc/file.png";
			add(make_control<ImageButton>(Image::loadImage(path),false),BorderLayout::WEST);
			add(make_control<Label>(file.name()),BorderLayout::CENTER);
		}
	};

	class IconFile : public FileObject {
	public:
		explicit IconFile(FileList &list,const esc::file &file) : FileObject(list,file) {
			using namespace gui;
			setLayout(make_layout<FlowLayout>(CENTER,true,VERTICAL,4));

			std::string path = file.is_dir() ? "/etc/folder.png" : "/etc/file.png";
			add(make_control<ImageButton>(Image::loadImage(path),false));
			add(make_control<Label>(file.name()));
		}
	};

public:
	enum Mode {
		MODE_LIST,
		MODE_ICONS,
	};

	explicit FileList(std::shared_ptr<PathBar> pathbar)
		 : Panel(), _path(), _pathbar(pathbar) {
	}

	void loadDir(const std::string &path) {
		try {
			_path = path;
			_pathbar->setPath(this,path);

			std::list<esc::file> files;
			std::vector<struct dirent> entries = esc::file(path).list_files(false);
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

	void setMode(Mode mode) {
		if(mode != _mode) {
			_mode = mode;
			loadDir(std::string(_path));
		}
	}

	void setList(const std::list<esc::file> &files) {
		using namespace gui;
		removeAll();

		if(_mode == MODE_LIST) {
			setLayout(make_layout<TableLayout>(3,files.size()));
			uint row = 0;
			for(auto it = files.begin(); it != files.end(); ++it, ++row) {
				add(make_control<ListFileName>(*this,*it),GridPos(0,row));

				char tmp[64];
				snprintf(tmp,sizeof(tmp),"%zu B",it->size());
				add(make_control<Label>(tmp,Align::BACK),GridPos(1,row));

				time_t modts = it->modified();
				struct tm *modtime = localtime(&modts);
				strftime(tmp,sizeof(tmp),"%m/%d/%Y at %H:%M:%S",modtime);
				add(make_control<Label>(tmp,Align::CENTER),GridPos(2,row));
			}
		}
		else {
			setLayout(make_layout<IconLayout>(HORIZONTAL,4));
			for(auto it = files.begin(); it != files.end(); ++it)
				add(gui::make_control<IconFile>(*this,*it));
		}

		layout();
		static_cast<gui::ScrollPane*>(getParent())->scrollToTop(false);
		repaint();
	}

	virtual void onMousePressed(const gui::MouseEvent &e) {
		if(e.isButton1Down()) {
			for(auto ctrl = begin(); ctrl != end(); ++ctrl) {
				FileObject *fo = dynamic_cast<FileObject*>(&**ctrl);
				if(fo) {
					bool hit;
					if(_mode == MODE_LIST) {
						hit = e.getPos().y >= fo->getPos().y &&
							  (gsize_t)e.getPos().y < fo->getPos().y + fo->getSize().height;
					}
					else {
						hit = e.getPos().x >= fo->getPos().x &&
							  (gsize_t)e.getPos().x < fo->getPos().x + fo->getSize().width &&
							  e.getPos().y >= fo->getPos().y &&
							  (gsize_t)e.getPos().y < fo->getPos().y + fo->getSize().height;
					}

					if(hit) {
						fo->onClick();
						break;
					}
				}
			}
		}
	}

private:
	Mode _mode;
	std::string _path;
	std::shared_ptr<PathBar> _pathbar;
};
