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
#include <sys/stat.h>
#include <stdio.h>
#include <map>

#include "ctrlcon.h"

// TODO maybe we should limit the cache size somehow

class DirCache {
	DirCache() = delete;

public:
	typedef std::map<std::string,struct stat> nodemap_type;

	struct List {
		std::string path;
		nodemap_type nodes;
	};

	typedef std::map<std::string,List*> dirmap_type;

	static List *getList(const CtrlConRef &ctrlRef,const char *path,bool load = true);
	static int getInfo(const CtrlConRef &ctrlRef,const char *path,struct stat *info);
	static void removeDirOf(const char *path);
	static void print(std::ostream &os);

private:
	static List *loadList(const CtrlConRef &ctrlRef,const char *dir);
	static List *findList(const char *path);
	static int find(List *list,const char *name,struct stat *info);
	static void insert(const char *path,struct stat *info);

	static std::string decode(const char *line,struct stat *info);
	static ino_t genINodeNo(const char *dir,const char *name);
	static int decodeMonth(const std::string &mon);
	static mode_t decodeMode(const std::string &mode);
	static mode_t decodePerm(const char *perms);

	static dirmap_type dirs;
	static time_t now;
};
