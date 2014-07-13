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

namespace esc {

class FileCopy {
public:
	enum {
		FL_FORCE		= 1 << 0,
		FL_PROGRESS		= 1 << 1,
		FL_RECURSIVE	= 1 << 2,
	};

	explicit FileCopy(size_t bufsize,uint fl);
	virtual ~FileCopy();

	bool copyFile(const char *src,const char *dest,bool remove = false);
	bool copy(const char *src,const char *dest,bool remove = false);
	bool move(const char *src,const char *dstdist,const char *filename);

	virtual void handleError(const char *,...) {
	}

private:
	ulong getTotalSteps() {
		return _cols - (25 + SSTRLEN(": 0000 KiB of 0000 KiB []"));
	}
	ulong getSteps(ulong totalSteps,size_t pos,size_t total) {
		return total == 0 ? 0 : ((ullong)totalSteps * pos) / total;
	}
	void showProgress(const char *src,size_t pos,size_t total,ulong steps,ulong totalSteps);
	void showSimpleProgress(const char *src,size_t size);
	void printSize(size_t size);

	size_t _bufsize;
	ulong _shmname;
	void *_shm;
	uint _flags;
	uint _cols;
};

}
