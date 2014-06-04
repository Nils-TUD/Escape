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

#include <esc/common.h>
#include <sstream>
#include <fstream>
#include <time.h>

#include "datacon.h"
#include "dircache.h"
#include "blockfile.h"

class DirList : public BlockFile {
public:
	explicit DirList(const std::string &path,const CtrlConRef &ctrlRef) : _os() {
		prepare(DirCache::getList(ctrlRef,path.c_str()));
	}

	virtual size_t read(void *buf,size_t offset,size_t count) {
		if(offset > _os.str().length() || offset + count < offset)
			return 0;
		if(offset + count > _os.str().length())
			count = _os.str().length() - offset;
		memcpy(buf,_os.str().c_str() + offset,count);
		return count;
	}

	virtual void write(const void *,size_t,size_t) {
		VTHROWE("You can't write to a directory",-ENOTSUP);
	}

private:
	void prepare(DirCache::List *list) {
		char buf[256];
		for(auto it = list->nodes.begin(); it != list->nodes.end(); ++it) {
			sDirEntry *e = (sDirEntry*)buf;
			e->nameLen = it->first.length();
			e->nodeNo = it->second.inodeNo;
			e->recLen = sizeof(*e) + it->first.length();
			memcpy(e->name,it->first.c_str(),it->first.length());
			_os.write((char*)e,e->recLen);
		}
	}

	std::ostringstream _os;
};
