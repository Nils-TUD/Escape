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

#include <esc/dir.h>
#include <file.h>
#include <string.h>
#include <env.h>
#include <errno.h>

namespace std {
	file::file(const string& p)
		: _info(sFileInfo()), _parent(string()), _name(string()) {
		init(p,"");
	}
	file::file(const string& p,const string& n)
		: _info(sFileInfo()), _parent(string()), _name(string()) {
		init(p,n);
	}
	file::file(const file& f)
		: _info(f._info), _parent(f._parent), _name(f._name) {
	}
	file& file::operator =(const file& f) {
		_info = f._info;
		_parent = f._parent;
		_name = f._name;
		return *this;
	}
	file::~file() {
	}

	vector<sDirEntry> file::list_files(bool showHidden,const string& pattern) const {
		vector<sDirEntry> v;
		sDirEntry e;
		if(!is_dir())
			throw io_exception("list_files failed: No directory",0);
		DIR *dir = opendir(path().c_str());
		if(dir == NULL)
			throw io_exception("opendir failed",errno);
		bool res;
		while((res = readdir(dir,&e))) {
			if((pattern.empty() || strmatch(pattern.c_str(),e.name)) &&
					(showHidden || e.name[0] != '.'))
				v.push_back(e);
		}
		closedir(dir);
		return v;
	}

	void file::init(const string& p,const string& n) {
		string apath(p);
		env::absolutify(apath);
		if(apath.size() > 1 && apath[apath.size() - 1] == '/')
			apath.erase(apath.end() - 1);
		if(!n.empty()) {
			if(apath.size() > 1)
				apath += "/";
			apath += n;
		}
		string::size_type pos = apath.rfind('/');
		if(!n.empty())
			_name = n;
		else
			_name = string(apath.begin() + pos + 1,apath.end());
		_parent = string(apath.begin(),apath.begin() + pos + 1);
		int res = stat(apath.c_str(),&_info);
		if(res < 0)
			throw io_exception("stat failed",res);
	}
}
