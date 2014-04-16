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
#include <usergroup/user.h>
#include <stdlib.h>
#include <stdio.h>

void *user_parseListFromFile(const char *file,size_t *count,parse_func parse) {
	long size;
	void *res = NULL;
	char *buf = NULL;
	FILE *f = fopen(file,"r");
	if(!f)
		return NULL;
	if(fseek(f,0,SEEK_END) < 0)
		goto error;
	size = ftell(f);
	if(fseek(f,0,SEEK_SET) < 0)
		goto error;
	buf = (char*)malloc(size + 1);
	if(!buf)
		goto error;
	if(fread(buf,1,size + 1,f) == 0)
		goto error;
	buf[size] = '\0';
	res = parse(buf,count);
error:
	free(buf);
	fclose(f);
	return res;
}

const char *user_readStr(char *dst,const char *src,size_t max,bool last) {
	size_t i = 0;
	char c;
	while((c = *src) && c != ':' && c != '\n' && i < max - 1) {
		dst[i++] = c;
		src++;
	}
	if(i == 0 || (!last && !*src))
		return NULL;
	if(!last)
		src++;
	dst[i] = '\0';
	return src;
}

sUser *user_parseFromFile(const char *file,size_t *count) {
	return (sUser*)user_parseListFromFile(file,count,(parse_func)user_parse);
}

sUser *user_parse(const char *users,size_t *count) {
	sUser *res = NULL;
	sUser *u,*last = NULL;
	const char *p = users;
	size_t cnt = 0;
	while(*p) {
		int uid,gid;
		if(sscanf(p,"%u:%u",&uid,&gid) != 2)
			goto error;
		/* to name */
		while(*p && *p != ':')
			p++;
		if(!*p)
			goto error;
		p++;
		while(*p && *p != ':')
			p++;
		if(!*p)
			goto error;
		p++;
		u = (sUser*)malloc(sizeof(sUser));
		if(!u)
			goto error;
		u->next = NULL;
		if(!res)
			res = u;
		if(last)
			last->next = u;
		u->uid = uid;
		u->gid = gid;
		if(!(p = user_readStr(u->name,p,sizeof(u->name),false)))
			goto error;
		if(!(p = user_readStr(u->home,p,sizeof(u->home),true)))
			goto error;
		/* to next line */
		while(*p && *p != '\n')
			p++;
		while(*p == '\n')
			p++;
		cnt = cnt + 1;
		last = u;
	}
	if(count)
		*count = cnt;
	return res;

error:
	user_free(res);
	return NULL;
}

void user_append(sUser *list,sUser *u) {
	sUser *ut = list;
	while(ut && ut->next)
		ut = ut->next;
	if(!ut)
		list->next = u;
	else
		ut->next = u;
	u->next = NULL;
}

void user_remove(sUser *list,sUser *u) {
	sUser *p = NULL;
	while(list != NULL) {
		if(list == u) {
			if(p)
				p->next = u->next;
			else
				list = u->next;
			break;
		}
		p = list;
		list = list->next;
	}
}

uid_t user_getFreeUid(const sUser *u) {
	uid_t res = 0;
	while(u != NULL) {
		if(u->uid > res)
			res = u->uid;
		u = u->next;
	}
	return res + 1;
}

sUser *user_getByName(const sUser *u,const char *name) {
	while(u != NULL) {
		if(strcmp(u->name,name) == 0)
			return (sUser*)u;
		u = u->next;
	}
	return NULL;
}

sUser *user_getById(const sUser *u,uid_t uid) {
	while(u != NULL) {
		if(u->uid == uid)
			return (sUser*)u;
		u = u->next;
	}
	return NULL;
}

int user_writeToFile(const sUser *u,const char *path) {
	int res;
	FILE *f = fopen(path,"w");
	if(!f)
		return errno;
	res = user_write(u,f);
	fclose(f);
	return res;
}

int user_write(const sUser *u,FILE *f) {
	while(u != NULL && !ferror(f)) {
		fprintf(f,"%u:%u:%s:%s\n",u->uid,u->gid,u->name,u->home);
		u = u->next;
	}
	return ferror(f);
}

void user_free(sUser *u) {
	while(u != NULL) {
		sUser *next = u->next;
		free(u);
		u = next;
	}
}

void user_print(const sUser *u) {
	while(u != NULL) {
		printf("\t%s(%d): ",u->name,u->uid);
		printf("\t\tdefaultGroup: %u\n",u->gid);
		printf("\t\thome: %s\n",u->home);
		u = u->next;
	}
}
