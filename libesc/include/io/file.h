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

#ifndef FILE_H_
#define FILE_H_

#include <esc/common.h>
#include <fsinterface.h>

typedef struct sFile sFile;

typedef sFileInfo *(*fGetInfo)(sFile *f);
typedef bool (*fIsFile)(sFile *f);
typedef bool (*fIsDir)(sFile *f);
typedef s32 (*fGetSize)(sFile *f);
typedef u32 (*fGetName)(sFile *f,char *buf,u32 size);
typedef u32 (*fGetParent)(sFile *f,char *buf,u32 size);
typedef u32 (*fGetAbsolute)(sFile *f,char *buf,u32 size);
typedef void (*fDestroy)(sFile *f);

struct sFile {
/* private: */
	sFileInfo _info;
	const char *_path;

/* public: */
	fIsFile isFile;
	fIsDir isDir;
	fGetName getName;
	fGetParent getParent;
	fGetAbsolute getAbsolute;
	fGetInfo getInfo;
	fGetSize getSize;
	fDestroy destroy;
};

sFile *file_get(const char *path);

#endif /* FILE_H_ */
