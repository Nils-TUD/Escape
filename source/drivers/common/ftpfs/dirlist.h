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

#include <sys/common.h>
#include <sys/endian.h>
#include <dirent.h>
#include <time.h>

#include "blockfile.h"
#include "datacon.h"
#include "dircache.h"

class DirList : public BlockFile {
public:
	explicit DirList(const std::string &path,const CtrlConRef &ctrlRef) : _buf(), _bufsize(), _buflen() {
		prepare(DirCache::getList(ctrlRef,path.c_str()));
	}
	virtual ~DirList() {
		delete[] _buf;
	}

	virtual size_t read(void *buf,size_t offset,size_t count) {
		if(offset > _buflen || offset + count < offset)
			return 0;
		if(offset + count > _buflen)
			count = _buflen - offset;
		memcpy(buf,_buf + offset,count);
		return count;
	}

	virtual void write(const void *,size_t,size_t) {
		VTHROWE("You can't write to a directory",-ENOTSUP);
	}

	virtual int sharemem(int,void *,size_t) {
		return -ENOTSUP;
	}

private:
	void prepare(DirCache::List *list) {
		size_t total = 0;
		for(auto it = list->nodes.begin(); it != list->nodes.end(); ++it)
			total += (sizeof(struct dirent) - (NAME_MAX + 1)) + it->first.length();

		_bufsize = total;
		_buf = new uint8_t[_bufsize];

		for(auto it = list->nodes.begin(); it != list->nodes.end(); ++it)
			append(it->first.c_str(),it->second.st_ino);
	}

	void append(const char *name,ino_t ino) {
		char buf[256];
		struct dirent *e = (struct dirent*)buf;
		size_t namelen = strlen(name);
		e->d_namelen = cputole16(namelen);
		e->d_ino = cputole32(ino);
		size_t reclen = (sizeof(*e) - (NAME_MAX + 1)) + namelen;
		e->d_reclen = cputole16(reclen);
		if(reclen <= sizeof(buf)) {
			memcpy(e->d_name,name,namelen);
			memcpy(_buf + _buflen,(char*)e,reclen);
			_buflen += reclen;
		}
	}

	uint8_t *_buf;
	size_t _bufsize;
	size_t _buflen;
};
