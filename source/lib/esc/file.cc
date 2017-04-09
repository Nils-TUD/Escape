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
#include <esc/file.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

namespace esc {
	file::file(const std::string& p,uint flags)
		: _info(), _parent(), _name() {
		init(p,"",flags);
	}
	file::file(const std::string& p,const std::string& n,uint flags)
		: _info(), _parent(), _name() {
		init(p,n,flags);
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

	std::vector<struct dirent> file::list_files(bool showHidden,const std::string& pattern) const {
		std::vector<struct dirent> v;
		struct dirent e;
		if(!is_dir())
			throw default_error("list_files failed: No directory",0);
		DIR *dir = opendir(path().c_str());
		if(dir == nullptr)
			throw default_error("opendir failed",errno);
		bool res;
		while((res = readdirto(dir,&e))) {
			if((pattern.empty() || strmatch(pattern.c_str(),e.d_name)) &&
					(showHidden || e.d_name[0] != '.'))
				v.push_back(e);
		}
		closedir(dir);
		return v;
	}

	void file::init(const std::string& p,const std::string& n,uint flags) {
		char apath[MAX_PATH_LEN];
		ssize_t len = cleanpath(apath,sizeof(apath),p.c_str());
		if(len < 0)
			throw default_error("cleanpath failed",len);

		if(n.empty()) {
			char *pos = strrchr(apath,'/');
			if(len == 1)
				_name = "";
			else {
				_name = std::string(pos + 1,apath + len);
				if(pos == apath)
					pos[1] = '\0';
				else
					pos[0] = '\0';
			}
		}
		else
			_name = n;
		_parent = apath;

		int fd = open(path().c_str(),flags);
		if(fd < 0)
			throw default_error("open failed",fd);
		int res = fstat(fd,&_info);
		close(fd);
		if(res < 0)
			throw default_error("fstat failed",res);
	}
}
