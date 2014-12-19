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
#include <sys/stat.h>
#include <sys/endian.h>
#include <esc/pathtree.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sstream>

struct TarINode {
	explicit TarINode(time_t mtime,off_t size,mode_t mode)
			: info(), offset(0), data(NULL), datasize(0), _refs(1) {
		info.st_mtime = mtime;
		info.st_atime = info.st_mtime;
		info.st_ctime = info.st_mtime;
		info.st_size = size;
		info.st_blocks = (info.st_size + 512 -1) / 512;
		info.st_blksize = 1024;
		info.st_ino = _inode++;
		info.st_mode = mode;
		info.st_nlink = 1;
	}
	~TarINode() {
		if(data)
			free(data);
	}

	void reference() {
		_refs++;
	}
	void deference() {
		if(--_refs == 0)
			delete this;
	}

	struct stat info;
	off_t offset;
	char *data;
	size_t datasize;

private:
	int _refs;
	static ino_t _inode;
};

class BlockFile {
public:
	virtual ~BlockFile() {
	}

	virtual ssize_t read(void *buf,size_t offset,size_t count) = 0;
	virtual ssize_t write(const void *buf,size_t offset,size_t count) = 0;
};

class RegularFile : public BlockFile {
public:
	explicit RegularFile(esc::PathTreeItem<TarINode> *item,FILE *archive,int flags)
		: _file(item->getData()), _archive(archive) {
		if(flags & O_TRUNC) {
			if(_file->data) {
				free(_file->data);
				_file->data = NULL;
			}
			item->getData()->info.st_size = 0;
			item->getData()->info.st_blocks = 0;
		}

		if(_file->data == NULL) {
			_file->datasize = MAX(1024,_file->info.st_size);
			_file->data = (char*)malloc(_file->datasize);
			if(_file->info.st_size > 0) {
				fseek(_archive,_file->offset,SEEK_SET);
				fread(_file->data,_file->info.st_size,1,_archive);
			}
		}
	}

	virtual ssize_t read(void *buf,size_t offset,size_t count) {
		if(offset + count < offset || (off_t)offset > _file->info.st_size)
			return -EINVAL;
		if((off_t)(offset + count) > _file->info.st_size)
			count = _file->info.st_size - offset;
		memcpy(buf,_file->data + offset,count);
		_file->info.st_atime = time(NULL);
		return count;
	}

	virtual ssize_t write(const void *buf,size_t offset,size_t count) {
		if(offset + count > _file->datasize) {
			char *ndata = (char*)realloc(_file->data,offset + count);
			if(!ndata)
				return -ENOMEM;
			_file->datasize = offset + count;
			_file->data = ndata;
		}
		if((off_t)(offset + count) > _file->info.st_size) {
			_file->info.st_size = offset + count;
			_file->info.st_blocks = (_file->info.st_size + 512 -1) / 512;
		}
		memcpy(_file->data + offset,buf,count);
		_file->info.st_mtime = time(NULL);
		return count;
	}

private:
	// don't need to add a reference here; OpenFile has one already
	TarINode *_file;
	FILE *_archive;
};

class DirFile : public BlockFile {
public:
	explicit DirFile(esc::PathTreeItem<TarINode> *item,esc::PathTree<TarINode> &tree) : _os() {
		char buf[256];

		// add "."
		struct dirent *e = (struct dirent*)buf;
		e->d_namelen = 1;
		e->d_ino = item->getData()->info.st_ino;
		e->d_reclen = cputole16((sizeof(*e) - (NAME_MAX + 1)) + 1);
		e->d_name[0] = '.';
		_os.write((char*)e,e->d_reclen);

		// add ".."
		e->d_namelen = 2;
		e->d_ino = item->getParent() != item ? item->getParent()->getData()->info.st_ino
									  		   : item->getData()->info.st_ino;
		e->d_reclen = cputole16((sizeof(*e) - (NAME_MAX + 1)) + 2);
		e->d_name[0] = '.';
		e->d_name[1] = '.';
		_os.write((char*)e,e->d_reclen);

		for(auto it = tree.begin(item); it != tree.end(); ++it) {
			struct dirent *e = (struct dirent*)buf;
			const char *name = it->getName();
			size_t namelen = strlen(name);
			e->d_namelen = cputole16(namelen);
			e->d_ino = cputole32(it->getData()->info.st_ino);
			e->d_reclen = cputole16((sizeof(*e) - (NAME_MAX + 1)) + namelen);
			memcpy(e->d_name,name,namelen);
			_os.write((char*)e,e->d_reclen);
		}
	}

	virtual ssize_t read(void *buf,size_t offset,size_t count) {
		if(offset > _os.str().length() || offset + count < offset)
			return 0;
		if(offset + count > _os.str().length())
			count = _os.str().length() - offset;
		memcpy(buf,_os.str().c_str() + offset,count);
		return count;
	}

	virtual ssize_t write(const void *,size_t,size_t) {
		return -ENOTSUP;
	}

private:
	std::ostringstream _os;
};
