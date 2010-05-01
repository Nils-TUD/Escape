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

#include <esc/common.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <mem/heap.h>
#include <exceptions/io.h>
#include <io/file.h>
#include <string.h>
#include <assert.h>

static bool file_isFile(sFile *f);
static bool file_isDir(sFile *f);
static sFileInfo *file_getInfo(sFile *f);
static s32 file_getSize(sFile *f);
static u32 file_getName(sFile *f,char *buf,u32 size);
static u32 file_getParent(sFile *f,char *buf,u32 size);
static u32 file_getAbsolute(sFile *f,char *buf,u32 size);
static void file_destroy(sFile *f);
static s32 file_getDirSlash(const char *path,u32 len);

sFile *file_get(const char *path) {
	s32 res;
	sFile *f = (sFile*)heap_alloc(sizeof(sFile));
	char *abs = (char*)heap_alloc(MAX_PATH_LEN);
	abspath(abs,MAX_PATH_LEN,path);
	f->_path = abs;
	res = stat(abs,&f->_info);
	if(res < 0)
		THROW(IOException,res);
	f->isFile = file_isFile;
	f->isDir = file_isDir;
	f->destroy = file_destroy;
	f->getAbsolute = file_getAbsolute;
	f->getName = file_getName;
	f->getParent = file_getParent;
	f->getInfo = file_getInfo;
	f->getSize = file_getSize;
	return f;
}

static bool file_isFile(sFile *f) {
	return MODE_IS_FILE(f->_info.mode);
}

static bool file_isDir(sFile *f) {
	return MODE_IS_DIR(f->_info.mode);
}

static sFileInfo *file_getInfo(sFile *f) {
	return &f->_info;
}

static s32 file_getSize(sFile *f) {
	return f->_info.size;
}

static u32 file_getName(sFile *f,char *buf,u32 size) {
	u32 res,len = strlen(f->_path);
	s32 lastSlash = file_getDirSlash(f->_path,len);
	assert(lastSlash != -1);
	res = MIN(size,len - (lastSlash + 2));
	strncpy(buf,f->_path + lastSlash + 1,res);
	buf[res] = '\0';
	return res;
}

static u32 file_getParent(sFile *f,char *buf,u32 size) {
	s32 len,lastSlash = file_getDirSlash(f->_path,strlen(f->_path));
	if(lastSlash <= 0) {
		len = MIN(1,size);
		strncpy(buf,"/",len);
	}
	else {
		len = MIN((s32)size,lastSlash + 1);
		strncpy(buf,f->_path,len);
	}
	buf[len] = '\0';
	return len;
}

static u32 file_getAbsolute(sFile *f,char *buf,u32 size) {
	u32 res = MIN(size,strlen(f->_path));
	strncpy(buf,f->_path,res);
	buf[res] = '\0';
	return res;
}

static void file_destroy(sFile *f) {
	heap_free((void*)f->_path);
	heap_free(f);
}

static s32 file_getDirSlash(const char *path,u32 len) {
	s32 i = len - 1;
	if(path[i] == '/')
		i--;
	for(; i >= 0; i--) {
		if(path[i] == '/')
			return i;
	}
	return -1;
}

