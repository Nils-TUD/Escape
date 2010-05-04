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

static sVector *file_listFiles(sFile *f,bool showHidden);
static sVector *file_listMatchingFiles(sFile *f,const char *pattern,bool showHidden);
static bool file_isFile(sFile *f);
static bool file_isDir(sFile *f);
static sFileInfo *file_getInfo(sFile *f);
static s32 file_getSize(sFile *f);
static const char *file_name(sFile *f);
static const char *file_parent(sFile *f);
static u32 file_getName(sFile *f,char *buf,u32 size);
static u32 file_getParent(sFile *f,char *buf,u32 size);
static u32 file_getAbsolute(sFile *f,char *buf,u32 size);
static void file_destroy(sFile *f);
static s32 file_getDirSlash(const char *path,u32 len);

sFile *file_get(const char *path) {
	return file_getIn(path,"");
}

sFile *file_getIn(const char *parent,const char *filename) {
	char tmp[MAX_PATH_LEN];
	s32 res;
	sFile *f = (sFile*)heap_alloc(sizeof(sFile));
	char *abs = (char*)heap_alloc(MAX_PATH_LEN);
	if(*filename) {
		s32 len = abspath(abs,MAX_PATH_LEN,parent);
		/* replace last '/' with '\0' if its not "/" */
		if(len > 1)
			abs[len - 1] = '\0';
		f->_name = (char*)heap_alloc(strlen(filename) + 1);
		strcpy(f->_name,filename);
	}
	else {
		u32 lastSlash;
		s32 len = abspath(abs,MAX_PATH_LEN,parent);
		lastSlash = file_getDirSlash(abs,len);
		abs[lastSlash] = '\0';
		f->_name = (char*)heap_alloc(len - lastSlash - 1);
		strncpy(f->_name,abs + lastSlash + 1,len - lastSlash - 2);
		f->_name[len - lastSlash - 2] = '\0';
		if(lastSlash == 0)
			strcpy(abs,"/");
	}
	f->_path = abs;
	file_getAbsolute(f,tmp,sizeof(tmp));
	res = stat(tmp,&f->_info);
	if(res < 0)
		THROW(IOException,res);
	f->listFiles = file_listFiles;
	f->listMatchingFiles = file_listMatchingFiles;
	f->isFile = file_isFile;
	f->isDir = file_isDir;
	f->destroy = file_destroy;
	f->getAbsolute = file_getAbsolute;
	f->name = file_name;
	f->parent = file_parent;
	f->getName = file_getName;
	f->getParent = file_getParent;
	f->getInfo = file_getInfo;
	f->getSize = file_getSize;
	return f;
}

static sVector *file_listFiles(sFile *f,bool showHidden) {
	return f->listMatchingFiles(f,NULL,showHidden);
}

static sVector *file_listMatchingFiles(sFile *f,const char *pattern,bool showHidden) {
	char path[MAX_PATH_LEN];
	bool res;
	sDirEntry *e;
	sVector *vec = vec_create(sizeof(sDirEntry*));
	f->getAbsolute(f,path,sizeof(path));
	tFD dir = opendir(path);
	if(dir < 0)
		THROW(IOException,dir);
	while(1) {
		e = heap_alloc(sizeof(sDirEntry));
		res = readdir(e,dir);
		if(!res) {
			heap_free(e);
			break;
		}
		if((pattern == NULL || strmatch(pattern,e->name)) && (showHidden || e->name[0] != '.'))
			vec_add(vec,&e);
		else
			heap_free(e);
	}
	closedir(dir);
	return vec;
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

static const char *file_name(sFile *f) {
	return f->_name;
}

static const char *file_parent(sFile *f) {
	return f->_path;
}

static u32 file_getName(sFile *f,char *buf,u32 size) {
	u32 len = strlen(f->_name);
	strncpy(buf,f->_name,size);
	buf[len] = '\0';
	return len;
}

static u32 file_getParent(sFile *f,char *buf,u32 size) {
	u32 len = strlen(f->_path);
	strncpy(buf,f->_path,size);
	buf[len] = '\0';
	return len;
}

static u32 file_getAbsolute(sFile *f,char *buf,u32 size) {
	u32 plen = strlen(f->_path),nlen = strlen(f->_name);
	strncpy(buf,f->_path,size);
	if(nlen && (plen + 1) < size) {
		buf[plen] = '/';
		strncpy(buf + plen + 1,f->_name,size - (plen + 1));
		buf[plen + nlen + 1] = '\0';
		return MIN(size - 1,plen + nlen + 1);
	}
	return MIN(size - 1,plen);
}

static void file_destroy(sFile *f) {
	heap_free((void*)f->_path);
	heap_free((void*)f->_name);
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
