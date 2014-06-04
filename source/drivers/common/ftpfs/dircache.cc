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

#include <esc/common.h>
#include <esc/dir.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "dircache.h"
#include "datacon.h"

DirCache::dirmap_type DirCache::dirs;
time_t DirCache::now = time(NULL);

DirCache::List *DirCache::getList(const CtrlConRef &ctrlRef,const char *path,bool load) {
	char tmppath[MAX_PATH_LEN];
	char cpath[MAX_PATH_LEN];
	// TODO maybe the kernel should send us the path with a slash at the beginning?
	snprintf(tmppath,sizeof(tmppath),"/%s",path);
	cleanpath(cpath,sizeof(cpath),tmppath);

	List *list = findList(cpath);
	if(!list && load)
		list = loadList(ctrlRef,cpath);
	return list;
}

int DirCache::getInfo(const CtrlConRef &ctrlRef,const char *path,sFileInfo *info) {
	char tmppath[MAX_PATH_LEN];
	char cpath[MAX_PATH_LEN];
	snprintf(tmppath,sizeof(tmppath),"/%s",path);
	cleanpath(cpath,sizeof(cpath),tmppath);

	const char *dir = dirname(cpath);
	dir = strcmp(dir,".") == 0 ? "/" : dir;
	List *list = findList(dir);
	if(!list)
		list = loadList(ctrlRef,dir);

	strnzcpy(tmppath,path,sizeof(tmppath));
	const char *filename = basename(tmppath);
	filename = strcmp(filename,"/") == 0 ? "." : filename;
	return find(list,filename,info);
}

void DirCache::removeDirOf(const char *path) {
	char tmppath[MAX_PATH_LEN];
	char cpath[MAX_PATH_LEN];
	snprintf(tmppath,sizeof(tmppath),"/%s",path);
	cleanpath(cpath,sizeof(cpath),tmppath);

	const char *dir = dirname(cpath);
	dirs.erase(dir);
}

DirCache::List *DirCache::loadList(const CtrlConRef &ctrlRef,const char *dir) {
	CtrlConRef myRef(ctrlRef);
	CtrlCon *ctrl = myRef.request();
	List *list = NULL;
	// new scope to close the data-socket before reading the reply from the ctrl socket
	{
		DataCon data(myRef);
		char tmppath[MAX_PATH_LEN];
		snprintf(tmppath,sizeof(tmppath),"-La %s",dir);
		ctrl->execute(CtrlCon::CMD_LIST,tmppath);

		list = new List;
		list->path = dir;
		char line[256];
		std::ifstream in;
		in.open(data.fd());
		while(!in.eof()) {
			sFileInfo finfo;
			in.getline(line,sizeof(line),'\n');
			if(line[0] == '\0')
				break;

			std::string name = decode(line,&finfo);
			finfo.inodeNo = genINodeNo(dir,name.c_str());
			list->nodes[name] = finfo;
		}
		dirs[dir] = list;
	}
	ctrl->readReply();
	return list;
}

DirCache::List *DirCache::findList(const char *path) {
	dirmap_type::iterator it = dirs.find(path);
	if(it == dirs.end())
		return NULL;
	return it->second;
}

int DirCache::find(List *list,const char *name,sFileInfo *info) {
	nodemap_type::iterator it = list->nodes.find(name);
	if(it == list->nodes.end())
		return -ENOENT;
	*info = it->second;
	return 0;
}

// TODO support other directory listings than UNIX-style listings

std::string DirCache::decode(const char *line,sFileInfo *info) {
	std::string perms,user,group,mon,name;
	int links = 0,day = 0,hour = 0;
	time_t ts;
	ulong size = 0;
	std::istringstream is(line);
	is >> perms >> links >> user >> group >> size >> mon >> day >> hour;
	int month = decodeMonth(mon);
	if(is.get() == ':') {
		int min = 0;
		is >> min;
		struct tm *nowtm = gmtime(&now);
		ts = timeof(month,day - 1,nowtm->tm_year,hour,min,0);
	}
	else
		ts = timeof(month,day - 1,hour - 1900,0,0,0);
	is >> name;
	info->blockCount = size / 1024;
	info->blockSize = 1024;
	info->gid = info->uid = 0;
	info->linkCount = links;
	info->mode = decodeMode(perms);
	info->size = size;
	info->accesstime = ts;
	info->modifytime = ts;
	info->createtime = ts;
	return name;
}

inode_t DirCache::genINodeNo(const char *dir,const char *name) {
	/* generate a more or less unique id from the path */
	inode_t no = 0;
	while(*dir)
		no = 31 * no + *dir++;
	while(*name)
		no = 31 * no + *name++;
    return no < 0 ? -no : no;
}

int DirCache::decodeMonth(const std::string &mon) {
	static const char *months[] = {
		"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
	};
	for(size_t i = 0; i < ARRAY_SIZE(months); ++i) {
		if(strcmp(months[i],mon.c_str()) == 0)
			return i;
	}
	// should not happen
	return 0;
}

mode_t DirCache::decodeMode(const std::string &mode) {
	if(mode.length() != 10)
		return 0;
	mode_t res = 0;
	if(mode[0] == 'd')
		res |= S_IFDIR;
	else
		res |= S_IFREG;
	res |= decodePerm(mode.c_str() + 1) << 6;
	res |= decodePerm(mode.c_str() + 4) << 3;
	res |= decodePerm(mode.c_str() + 7) << 0;
	return res;
}

mode_t DirCache::decodePerm(const char *perms) {
	mode_t res = 0;
	for(int i = 0; i < 3; ++i) {
		if(perms[i] != '-')
			res |= 1 << (2 - i);
	}
	return res;
}

void DirCache::print(std::ostream &os) {
	os << "Directory lists:\n";
	for(auto it = dirs.begin(); it != dirs.end(); ++it) {
		os << "  " << it->first << ":\n";
		for(auto n = it->second->nodes.begin(); n != it->second->nodes.end(); ++n)
			os << "    " << n->first << " (" << n->second.inodeNo << "," << n->second.size << ")\n";
	}
}
