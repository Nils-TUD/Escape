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
		: _info(sFileInfo()), _path(string()) {
		init(p,"");
	}
	file::file(const string& p,const string& n)
		: _info(sFileInfo()), _path(string()) {
		init(p,n);
	}
	file::file(const file& f)
		: _info(f._info), _path(f._path) {
	}
	file& file::operator =(const file& f) {
		_info = f._info;
		_path = f._path;
		return *this;
	}
	file::~file() {
	}

	vector<sDirEntry> file::list_files(bool showHidden,const string& pattern) {
		vector<sDirEntry> v;
		sDirEntry e;
		tFD dir = opendir(_path.c_str());
		if(dir < 0)
			throw file_error("opendir failed",dir);
		bool res;
		while((res = readdir(&e,dir))) {
			if((pattern.empty() || strmatch(pattern.c_str(),e.name)) &&
					(showHidden || e.name[0] != '.'))
				v.push_back(e);
		}
		closedir(dir);
		return v;
	}

	string file::name() const {
		string::size_type pos = _path.rfind('/');
		if(pos == 0)
			return "";
		return string(_path.begin() + pos + 1,_path.end());
	}
	string file::parent() const {
		string::size_type pos2,pos1 = _path.rfind('/');
		if(pos1 == 0)
			return "/";
		pos2 = _path.rfind('/',pos1);
		return string(_path.begin() + pos2 + 1,_path.begin() + pos1);
	}

	void file::init(const string& p,const string& n) {
		char tmp[MAX_PATH_LEN];
		if(!n.empty()) {
			_path = p;
			if(!_path.empty() && _path[_path.size() - 1] == '/')
				_path.erase(_path.end() - 1,_path.end());
			_path += n;
		}
		else
			_path = p;
		abspath(tmp,sizeof(tmp),_path.c_str());
		_path = tmp;
		s32 res = stat(tmp,&_info);
		if(res < 0)
			throw file_error("stat failed",res);
	}
}
