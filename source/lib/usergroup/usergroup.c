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

#include <usergroup/usergroup.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

sNamedItem *usergroup_parse(const char *path,size_t *count) {
	sNamedItem *res = NULL;
	sNamedItem *i,*last = NULL;
	size_t cnt = 0;

	DIR *d = opendir(path);
	if(!d)
		return NULL;

	struct dirent e;
	while(readdirto(d,&e)) {
		if(strcmp(e.d_name,".") == 0 || strcmp(e.d_name,"..") == 0)
			continue;

		i = (sNamedItem*)malloc(sizeof(sNamedItem));
		if(!i)
			goto error;
		i->next = NULL;
		if(!res)
			res = i;
		if(last)
			last->next = i;

		strnzcpy(i->name,e.d_name,sizeof(i->name));
		i->id = usergroup_loadIntAttr(path,i->name,"id");
		if(i->id < 0)
			goto error;

		cnt = cnt + 1;
		last = i;
	}
	closedir(d);
	if(count)
		*count = cnt;
	return res;

error:
	closedir(d);
	usergroup_free(res);
	return NULL;
}

int usergroup_getFreeId(const sNamedItem *i) {
	int res = 0;
	while(i != NULL) {
		if(i->id > res)
			res = i->id;
		i = i->next;
	}
	return res + 1;
}

sNamedItem *usergroup_getByName(const sNamedItem *i,const char *name) {
	while(i != NULL) {
		if(strcmp(i->name,name) == 0)
			return (sNamedItem*)i;
		i = i->next;
	}
	return NULL;
}

sNamedItem *usergroup_getById(const sNamedItem *i,int id) {
	while(i != NULL) {
		if(i->id == id)
			return (sNamedItem*)i;
		i = i->next;
	}
	return NULL;
}

bool usergroup_groupInUse(gid_t gid) {
	DIR *d = opendir(USERS_PATH);
	if(!d)
		return false;

	bool res = false;
	struct dirent e;
	while(!res && readdirto(d,&e)) {
		if(strcmp(e.d_name,".") == 0 || strcmp(e.d_name,"..") == 0)
			continue;

		size_t count;
		gid_t *groups = usergroup_collectGroupsFor(e.d_name,0,&count);
		for(size_t i = 0; i < count; ++i) {
			if(groups[i] == gid) {
				res = true;
				break;
			}
		}
		free(groups);
	}
	closedir(d);
	return res;
}

gid_t *usergroup_collectGroupsFor(const char *user,size_t openSlots,size_t *count) {
	char buf[256];
	if(usergroup_loadStrAttr(USERS_PATH,user,"groups",buf,sizeof(buf)) != 0)
		return NULL;

	size_t size = MAX(openSlots,8);
	gid_t *res = (gid_t*)malloc(sizeof(gid_t) * size);
	if(!res)
		return NULL;

	*count = 0;
	char *p = buf;
	while(*p) {
		char *end;
		int gid = strtoul(p,&end,10);

		if(*count + openSlots >= size) {
			gid_t *old = res;
			size *= 2;
			res = (gid_t*)realloc(res,sizeof(gid_t) * size);
			if(!res) {
				free(old);
				return NULL;
			}
		}
		res[*count] = gid;
		*count = *count + 1;

		if(!*end)
			break;
		p = end + 1;
	}
	return res;
}

void usergroup_free(sNamedItem *i) {
	while(i != NULL) {
		sNamedItem *next = i->next;
		free(i);
		i = next;
	}
}

void usergroup_print(const sNamedItem *i) {
	while(i != NULL) {
		printf("\t%s=%d\n",i->name,i->id);
		i = i->next;
	}
}

const char *usergroup_idToName(const char *folder,int id) {
	static char name[MAX_PATH_LEN];
	DIR *d = opendir(folder);
	if(!d)
		return NULL;
	struct dirent e;
	while(readdirto(d,&e)) {
		if(strcmp(e.d_name,".") == 0 || strcmp(e.d_name,"..") == 0)
			continue;

		int nid = usergroup_loadIntAttr(folder,e.d_name,"id");
		if(nid == id) {
			strnzcpy(name,e.d_name,sizeof(name));
			closedir(d);
			return name;
		}
	}
	closedir(d);
	return NULL;
}

int usergroup_loadIntAttr(const char *folder,const char *name,const char *attr) {
	char val[12];
	int res = usergroup_loadStrAttr(folder,name,attr,val,sizeof(val));
	if(res < 0)
		return res;
	return atoi(val);
}

int usergroup_storeIntAttr(const char *folder,const char *name,const char *attr,int value,
		mode_t mode) {
	char val[12];
	itoa(val,sizeof(val),value);
	return usergroup_storeStrAttr(folder,name,attr,val,mode);
}

int usergroup_loadStrAttr(const char *folder,const char *name,const char *attr,char *buf,size_t size) {
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"%s/%s/%s",folder,name,attr);

	int fd = open(path,O_RDONLY);
	if(fd < 0)
		return fd;
	ssize_t res = read(fd,buf,size - 1);
	close(fd);
	if(res < 0)
		return res;
	buf[res] = '\0';
	return 0;
}

int usergroup_storeStrAttr(const char *folder,const char *name,const char *attr,const char *str,
		mode_t mode) {
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"%s/%s/%s",folder,name,attr);

	int fd = creat(path,mode);
	if(fd < 0)
		return fd;
	ssize_t res = write(fd,str,strlen(str) + 1);
	close(fd);
	if(res < 0)
		return res;
	return 0;
}
