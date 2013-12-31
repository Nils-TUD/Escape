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
		: _info(), _parent(), _name() {
		init(p,"");
	}
	file::file(const string& p,const string& n)
		: _info(), _parent(), _name() {
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
		if(dir == nullptr)
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
		char apath[MAX_PATH_LEN];
		size_t len = abspath(apath,sizeof(apath),p.c_str());
		if(len > 1) {
			/* remove ending slash */
			apath[len - 1] = '\0';
		}
		if(n.empty()) {
			char *pos = strrchr(apath,'/');
			if(len == 1)
				_name = "";
			else {
				_name = string(pos + 1,apath + len);
				if(pos == apath)
					pos[1] = '\0';
				else
					pos[0] = '\0';
			}
		}
		else
			_name = n;
		_parent = apath;
		int res = stat(path().c_str(),&_info);
		if(res < 0)
			throw io_exception("stat failed",-res);
	}
}
