/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <file.h>
#include <string.h>
#include <esc/dir.h>

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
		tFD dir = opendir(path().c_str());
		if(dir < 0)
			throw io_exception("opendir failed",dir);
		bool res;
		while((res = readdir(&e,dir))) {
			if((pattern.empty() || strmatch(pattern.c_str(),e.name)) &&
					(showHidden || e.name[0] != '.'))
				v.push_back(e);
		}
		closedir(dir);
		return v;
	}

	void file::init(const string& p,const string& n) {
		char tmp[MAX_PATH_LEN];
		string apath(p);
		apath += '/';
		apath += n;
		// TODO this does not work for "." and ".." because abspath "resolves" them
		// this way we would for example stat "/" for "/boot/.." instead
		u32 len = abspath(tmp,sizeof(tmp),apath.c_str());
		if(len > 1)
			tmp[len - 1] = '\0';
		char *last = strrchr(tmp,'/');
		string::size_type pos = last - tmp;
		if(pos == 0)
			_parent = '/';
		else
			_parent = string(tmp,tmp + pos);
		if(n == "." || n == "..")
			_name = n;
		else
			_name = string(tmp + pos + 1,tmp + len);
		s32 res = stat(tmp,&_info);
		if(res < 0)
			throw io_exception("stat failed",res);
	}
}
