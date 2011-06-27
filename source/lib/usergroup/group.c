/**
 * $Id$
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
		while(*p && *p != ':' && i < MAX_GROUPNAME_LEN - 1)
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

gid_t *group_collectGroupsFor(const sGroup *g,uid_t uid,size_t *count) {
	size_t i,size = 8;
	gid_t *res = (gid_t*)malloc(sizeof(gid_t) * size);
	if(!res)
		return NULL;
	*count = 0;
	while(g != NULL) {
		for(i = 0; i < g->userCount; i++) {
			if(g->users[i] == uid) {
				if(*count >= size) {
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
