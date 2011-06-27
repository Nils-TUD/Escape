/**
 * $Id$
 */

#include <esc/common.h>
#include <usergroup/user.h>
#include <stdlib.h>
#include <stdio.h>

static const char *user_readStr(char *dst,const char *src,size_t max,bool last);

sUser *user_parseFromFile(const char *file,size_t *count) {
	long size;
	sUser *res = NULL;
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
	res = user_parse(buf,count);
error:
	free(buf);
	fclose(f);
	return res;
}

sUser *user_parse(const char *users,size_t *count) {
	sUser *res = NULL;
	sUser *u,*last = NULL;
	const char *p = users;
	*count = 0;
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
		if(!(p = user_readStr(u->name,p,MAX_USERNAME_LEN,false)))
			goto error;
		if(!(p = user_readStr(u->pw,p,MAX_PW_LEN,false)))
			goto error;
		if(!(p = user_readStr(u->home,p,MAX_PATH_LEN,true)))
			goto error;
		/* to next line */
		while(*p && *p != '\n')
			p++;
		while(*p == '\n')
			p++;
		*count = *count + 1;
		last = u;
	}
	return res;

error:
	user_free(res);
	return NULL;
}

sUser *user_getById(sUser *u,uid_t uid) {
	while(u != NULL) {
		if(u->uid == uid)
			return u;
		u = u->next;
	}
	return NULL;
}

int user_write(const sUser *u,FILE *f) {
	while(u != NULL && !ferror(f)) {
		fprintf(f,"%u:%u:%s:%s:%s\n",u->uid,u->gid,u->name,u->pw,u->home);
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
		printf("\t\tpw: %s\n",u->pw);
		printf("\t\thome: %s\n",u->home);
		u = u->next;
	}
}

static const char *user_readStr(char *dst,const char *src,size_t max,bool last) {
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
