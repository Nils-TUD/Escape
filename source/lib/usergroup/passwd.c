/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <esc/common.h>
#include <usergroup/passwd.h>
#include <usergroup/user.h>
#include <stdlib.h>
#include <stdio.h>

sPasswd *pw_parseFromFile(const char *file,size_t *count) {
	return (sPasswd*)user_parseListFromFile(file,count,(parse_func)pw_parse);
}

sPasswd *pw_parse(const char *pws,size_t *count) {
	sPasswd *res = NULL;
	sPasswd *pw,*last = NULL;
	const char *p = pws;
	size_t cnt = 0;
	while(*p) {
		int uid;
		if(sscanf(p,"%u",&uid) != 1)
			goto error;
		/* to password */
		while(*p && *p != ':')
			p++;
		if(!*p)
			goto error;
		p++;
		pw = (sPasswd*)malloc(sizeof(sPasswd));
		if(!pw)
			goto error;
		pw->next = NULL;
		if(!res)
			res = pw;
		if(last)
			last->next = pw;
		pw->uid = uid;
		if(!(p = user_readStr(pw->pw,p,sizeof(pw->pw),true)))
			goto error;
		/* to next line */
		while(*p && *p != '\n')
			p++;
		while(*p == '\n')
			p++;
		cnt = cnt + 1;
		last = pw;
	}
	if(count)
		*count = cnt;
	return res;

error:
	pw_free(res);
	return NULL;
}

void pw_append(sPasswd *list,sPasswd *pw) {
	sPasswd *ut = list;
	while(ut && ut->next)
		ut = ut->next;
	if(!ut)
		list->next = pw;
	else
		ut->next = pw;
	pw->next = NULL;
}

void pw_remove(sPasswd *list,sPasswd *pw) {
	sPasswd *p = NULL;
	while(list != NULL) {
		if(list == pw) {
			if(p)
				p->next = pw->next;
			else
				list = pw->next;
			break;
		}
		p = list;
		list = list->next;
	}
}

sPasswd *pw_getById(const sPasswd *pws,uid_t uid) {
	while(pws != NULL) {
		if(pws->uid == uid)
			return (sPasswd*)pws;
		pws = pws->next;
	}
	return NULL;
}

int pw_writeToFile(const sPasswd *pws,const char *path) {
	int res;
	FILE *f = fopen(path,"w");
	if(!f)
		return -errno;
	res = pw_write(pws,f);
	fclose(f);
	return res;
}

int pw_write(const sPasswd *pws,FILE *f) {
	while(pws != NULL && !ferror(f)) {
		fprintf(f,"%u:%s\n",pws->uid,pws->pw);
		pws = pws->next;
	}
	return ferror(f);
}

void pw_free(sPasswd *pws) {
	while(pws != NULL) {
		sPasswd *next = pws->next;
		free(pws);
		pws = next;
	}
}

void pw_print(const sPasswd *pws) {
	while(pws != NULL) {
		printf("\t%d: ",pws->uid);
		printf("\t\tpw: %s\n",pws->pw);
		pws = pws->next;
	}
}
