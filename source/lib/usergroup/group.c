/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <usergroup/group.h>
#include <stdlib.h>
#include <stdio.h>

sGroup *group_parseFromFile(const char *file,size_t *count) {
	long size;
	sGroup *res = NULL;
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
	res = group_parse(buf,count);
error:
	free(buf);
	fclose(f);
	return res;
}

sGroup *group_parse(const char *groups,size_t *count) {
	sGroup *res = NULL;
	sGroup *g,*last = NULL;
	const char *p = groups;
	*count = 0;
	while(*p) {
		size_t i,size;
		int uid,gid;
		if(sscanf(p,"%u",&gid) != 1)
			goto error;
		/* to name */
		while(*p && *p != ':')
			p++;
		if(!*p)
			goto error;
		p++;
		g = (sGroup*)malloc(sizeof(sGroup));
		if(!g)
			goto error;
		g->next = NULL;
		if(!res)
			res = g;
		if(last)
			last->next = g;
		g->gid = gid;
		g->userCount = 0;
		size = 8;
		g->users = (uid_t*)malloc(size * sizeof(uid_t));
		if(!g->users)
			goto error;
		/* read the name */
		i = 0;
		while(*p && *p != '\n' && *p != ':' && i < MAX_GROUPNAME_LEN)
			g->name[i++] = *p++;
		/* empty name is not allowed */
		if(i == 0)
			goto error;
		g->name[i] = '\0';
		/* read user-ids */
		while(p && sscanf(p,":%u",&uid) == 1) {
			if(g->userCount >= size) {
				uid_t *old = g->users;
				size *= 2;
				g->users = (uid_t*)realloc(g->users,size * sizeof(uid_t));
				if(!g->users) {
					free(old);
					goto error;
				}
			}
			g->users[g->userCount++] = uid;
			p++;
			while(*p && *p != ':' && *p != '\n')
				p++;
		}
		/* to next line */
		while(*p && *p != '\n')
			p++;
		while(*p == '\n')
			p++;
		*count = *count + 1;
		last = g;
	}
	return res;

error:
	group_free(res);
	return NULL;
}

void group_append(sGroup *list,sGroup *g) {
	sGroup *gt = list;
	while(gt && gt->next)
		gt = gt->next;
	if(!gt)
		list->next = g;
	else
		gt->next = g;
	g->next = NULL;
}

void group_remove(sGroup *list,sGroup *g) {
	sGroup *p = NULL;
	while(list != NULL) {
		if(list == g) {
			if(p)
				p->next = g->next;
			else
				list = g->next;
			break;
		}
		p = list;
		list = list->next;
	}
}

gid_t group_getFreeGid(const sGroup *g) {
	gid_t res = 0;
	while(g != NULL) {
		if(g->gid > res)
			res = g->gid;
		g = g->next;
	}
	return res + 1;
}

sGroup *group_getById(const sGroup *g,gid_t gid) {
	while(g != NULL) {
		if(g->gid == gid)
			return (sGroup*)g;
		g = g->next;
	}
	return NULL;
}

sGroup *group_getByName(const sGroup *g,const char *name) {
	while(g != NULL) {
		if(strcmp(g->name,name) == 0)
			return (sGroup*)g;
		g = g->next;
	}
	return NULL;
}

gid_t *group_collectGroupsFor(const sGroup *g,uid_t uid,size_t openSlots,size_t *count) {
	size_t i,size = MAX(openSlots,8);
	gid_t *res = (gid_t*)malloc(sizeof(gid_t) * size);
	if(!res)
		return NULL;
	*count = 0;
	while(g != NULL) {
		for(i = 0; i < g->userCount; i++) {
			if(g->users[i] == uid) {
				if(*count + openSlots >= size) {
					gid_t *old = res;
					size *= 2;
					res = (gid_t*)realloc(res,sizeof(gid_t) * size);
					if(!res) {
						free(old);
						return NULL;
					}
				}
				res[*count] = g->gid;
				*count = *count + 1;
			}
		}
		g = g->next;
	}
	return res;
}

void group_removeFrom(sGroup *g,uid_t uid) {
	size_t i,j;
	for(i = 0, j = 0; i < g->userCount; i++) {
		g->users[j] = g->users[i];
		if(g->users[i] != uid)
			j++;
	}
	g->userCount = j;
}

void group_removeFromAll(sGroup *g,uid_t uid) {
	while(g != NULL) {
		group_removeFrom(g,uid);
		g = g->next;
	}
}

int group_writeToFile(const sGroup *g,const char *path) {
	int res;
	FILE *f = fopen(path,"w");
	if(!f)
		return errno;
	res = group_write(g,f);
	fclose(f);
	return res;
}

int group_write(const sGroup *g,FILE *f) {
	size_t i;
	while(g != NULL && !ferror(f)) {
		fprintf(f,"%u:%s",g->gid,g->name);
		for(i = 0; i < g->userCount; i++)
			fprintf(f,":%u",g->users[i]);
		putc('\n',f);
		g = g->next;
	}
	return ferror(f);
}

void group_free(sGroup *g) {
	while(g != NULL) {
		sGroup *next = g->next;
		free(g->users);
		free(g);
		g = next;
	}
}

void group_print(const sGroup *g) {
	while(g != NULL) {
		size_t i;
		printf("\t%s(%d): ",g->name,g->gid);
		for(i = 0; i < g->userCount; i++)
			printf("%u ",g->users[i]);
		printf("\n");
		g = g->next;
	}
}
