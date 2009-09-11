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

#include <types.h>
#include <app.h>
#include <string.h>
#include <ctype.h>

#ifdef IN_KERNEL
#	include <mem/kheap.h>
#	include <video.h>
#	define apprintf		vid_printf
#	define free(x)		kheap_free(x)
#	define malloc(x)	kheap_alloc(x)
#else
#	include <esc/heap.h>
#	include <esc/fileio.h>
#	define apprintf printf
#endif

#define MAX_TOKEN_LEN	30

#define TOK_STRING		0
#define TOK_INT			1
#define TOK_SEM			2
#define TOK_NAME		3
#define TOK_COMMA		4
#define TOK_COL			5
#define TOK_DOTDOT		6
#define TOK_EOF			7

typedef struct {
	const char *str;
	u32 tokLen;
	u32 tokType;
	char token[MAX_TOKEN_LEN + 1];
} sParseInfo;

static bool app_drvsToStr(sStringBuffer *str,sSLList *list);
static bool app_intsToStr(sStringBuffer *str,sSLList *list);
static bool app_strsToStr(sStringBuffer *str,sSLList *list);
static bool app_rangesToStr(sStringBuffer *str,sSLList *list);

static bool app_nextToken(sParseInfo *info);
static bool app_parseDriverList(sParseInfo *info,sSLList *list);
static bool app_parseDriver(sParseInfo *info,sDriverPerm *drv);
static bool app_parseIntList(sParseInfo *info,sSLList *list);
static bool app_parseStrList(sParseInfo *info,sSLList *list);
static bool app_parseRangeList(sParseInfo *info,sSLList *list);
static bool app_parseRange(sParseInfo *info,sSLList *list);

#define ADD_PERM(str,fmt,...)						\
	do {											\
		if(!asprintf((str),(fmt),## __VA_ARGS__))	\
			return false;							\
	}												\
	while(0);

#define ADD_LPERM(str,fmt,func,list)				\
	do {											\
		if(!asprintf((str),(fmt)))					\
			return false;							\
		if(!(func)((str),(list)))					\
			return false;							\
		if(!asprintf((str),";\n"))					\
			return false;							\
	}												\
	while(0);

bool app_toString(sStringBuffer *str,sApp *app,char *src,bool srcWritable) {
	const char *types[] = {"driver","service","fs","default"};
	ADD_PERM(str,  "source:					\"%s\";\n",src);
	ADD_PERM(str,  "sourceWritable:			%d;\n",srcWritable);
	ADD_PERM(str,  "type:					\"%s\";\n",types[app->appType]);
	ADD_LPERM(str, "ioports:				",app_rangesToStr,app->ioports);
	ADD_LPERM(str, "driver:\n",app_drvsToStr,app->driver);
	ADD_PERM(str,  "fs:						%d,%d;\n",app->fs.read,app->fs.write);
	ADD_LPERM(str, "services:				",app_strsToStr,app->services);
	ADD_LPERM(str, "signals:				",app_intsToStr,app->signals);
	ADD_LPERM(str, "physmem:				",app_rangesToStr,app->physMem);
	ADD_LPERM(str, "crtshmem:				",app_strsToStr,app->createShMem);
	ADD_LPERM(str, "joinshmem:				",app_strsToStr,app->joinShMem);
	return true;
}

static bool app_drvsToStr(sStringBuffer *str,sSLList *list) {
	const char *name;
	sSLNode *n;
	sDriverPerm *drv;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		drv = (sDriverPerm*)n->data;
		if(drv->group == DRV_GROUP_BINPRIV)
			name = "BINPRIV";
		else if(drv->group == DRV_GROUP_BINPUB)
			name = "BINPUB";
		else if(drv->group == DRV_GROUP_TXTPRIV)
			name = "TXTPRIV";
		else if(drv->group == DRV_GROUP_TXTPUB)
			name = "TXTPUB";
		else
			name = drv->name;
		ADD_PERM(str,"\t\"%s\",%d,%d,%d%s",name,drv->read,drv->write,drv->ioctrl,
				n->next != NULL ? ",\n" : "");
	}
	return true;
}

static bool app_intsToStr(sStringBuffer *str,sSLList *list) {
	sSLNode *n;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		ADD_PERM(str,"%d",(u32)n->data);
		if(n->next != NULL)
			ADD_PERM(str,",");
	}
	return true;
}

static bool app_strsToStr(sStringBuffer *str,sSLList *list) {
	sSLNode *n;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		ADD_PERM(str,"\"%s\"",(char*)n->data);
		if(n->next != NULL)
			ADD_PERM(str,",");
	}
	return true;
}

static bool app_rangesToStr(sStringBuffer *str,sSLList *list) {
	sSLNode *n;
	sRange *r;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		r = (sRange*)n->data;
		ADD_PERM(str,"%d..%d",r->start,r->end);
		if(n->next != NULL)
			ADD_PERM(str,",");
	}
	return true;
}

bool app_fromString(const char *definition,sApp *app,char *src,bool *srcWritable) {
	sParseInfo info;
	info.str = definition;
	app->db = NULL;
	app->id = 0;
	app->appType = APP_TYPE_DEFAULT;
	app->fs.read = false;
	app->fs.write = false;
	app->createShMem = NULL;
	app->joinShMem = NULL;
	app->driver = NULL;
	app->ioports = NULL;
	app->services = NULL;
	app->physMem = NULL;
	app->signals = NULL;
	while(1) {
		if(!app_nextToken(&info)) {
			if(info.tokType == TOK_EOF)
				break;
			goto error;
		}
		if(info.tokType != TOK_NAME)
			goto error;

		if(strcmp(info.token,"source") == 0) {
			if(!app_nextToken(&info) || info.tokType != TOK_STRING)
				goto error;
			strcpy(src,info.token);
			if(!app_nextToken(&info) || info.tokType != TOK_SEM)
				goto error;
		}
		else if(strcmp(info.token,"sourceWritable") == 0) {
			if(!app_nextToken(&info) || info.tokType != TOK_INT)
				goto error;
			*srcWritable = info.tokLen > 0 && info.token[0] == '1';
			if(!app_nextToken(&info) || info.tokType != TOK_SEM)
				goto error;
		}
		else if(strcmp(info.token,"type") == 0) {
			if(!app_nextToken(&info) || info.tokType != TOK_STRING)
				goto error;
			if(strcmp(info.token,"driver") == 0)
				app->appType = APP_TYPE_DRIVER;
			else if(strcmp(info.token,"service") == 0)
				app->appType = APP_TYPE_SERVICE;
			else if(strcmp(info.token,"fs") == 0)
				app->appType = APP_TYPE_FS;
			else if(strcmp(info.token,"default") == 0)
				app->appType = APP_TYPE_DEFAULT;
			else
				goto error;
			if(!app_nextToken(&info) || info.tokType != TOK_SEM)
				goto error;
		}
		else if(strcmp(info.token,"ioports") == 0) {
			app->ioports = sll_create();
			if(!app->ioports)
				goto error;
			if(!app_parseRangeList(&info,app->ioports))
				goto error;
		}
		else if(strcmp(info.token,"driver") == 0) {
			app->driver = sll_create();
			if(!app->driver)
				goto error;
			if(!app_parseDriverList(&info,app->driver))
				goto error;
		}
		else if(strcmp(info.token,"fs") == 0) {
			if(!app_nextToken(&info) || info.tokType != TOK_INT)
				goto error;
			app->fs.read = info.tokLen > 0 && info.token[0] == '1';
			if(!app_nextToken(&info) || info.tokType != TOK_COMMA)
				goto error;
			if(!app_nextToken(&info) || info.tokType != TOK_INT)
				goto error;
			app->fs.write = info.tokLen > 0 && info.token[0] == '1';
			if(!app_nextToken(&info) || info.tokType != TOK_SEM)
				goto error;
		}
		else if(strcmp(info.token,"services") == 0) {
			app->services = sll_create();
			if(!app->services)
				goto error;
			if(!app_parseStrList(&info,app->services))
				goto error;
		}
		else if(strcmp(info.token,"signals") == 0) {
			app->signals = sll_create();
			if(!app->signals)
				goto error;
			if(!app_parseIntList(&info,app->signals))
				goto error;
		}
		else if(strcmp(info.token,"physmem") == 0) {
			app->physMem = sll_create();
			if(!app->physMem)
				goto error;
			if(!app_parseRangeList(&info,app->physMem))
				goto error;
		}
		else if(strcmp(info.token,"crtshmem") == 0) {
			app->createShMem = sll_create();
			if(!app->createShMem)
				goto error;
			if(!app_parseStrList(&info,app->createShMem))
				goto error;
		}
		else if(strcmp(info.token,"joinshmem") == 0) {
			app->joinShMem = sll_create();
			if(!app->joinShMem)
				goto error;
			if(!app_parseStrList(&info,app->joinShMem))
				goto error;
		}
		else
			goto error;
	}
	return true;

error:
	sll_destroy(app->createShMem,true);
	app->createShMem = NULL;
	sll_destroy(app->joinShMem,true);
	app->joinShMem = NULL;
	sll_destroy(app->driver,true);
	app->driver = NULL;
	sll_destroy(app->ioports,true);
	app->ioports = NULL;
	sll_destroy(app->services,true);
	app->services = NULL;
	sll_destroy(app->physMem,true);
	app->physMem = NULL;
	sll_destroy(app->signals,false);
	app->signals = NULL;
	return false;
}

static bool app_nextToken(sParseInfo *info) {
	char c;
	info->tokLen = 0;
	while(1) {
		c = *info->str;
		if(c == '\0') {
			info->tokType = TOK_EOF;
			return false;
		}
		/* whitespace is ignored */
		if(isspace(c)) {
			info->str++;
			continue;
		}

		/* dotdot: '..' */
		if(c == '.' && *(info->str + 1) == '.') {
			info->token[info->tokLen++] = c;
			info->token[info->tokLen++] = *(info->str + 1);
			info->token[info->tokLen] = '\0';
			info->tokType = TOK_DOTDOT;
			info->str += 2;
			break;
		}

		/* special char */
		if(c == ';' || c == ',') {
			info->token[info->tokLen++] = c;
			info->token[info->tokLen] = '\0';
			info->tokType = c == ';' ? TOK_SEM : TOK_COMMA;
			info->str++;
			break;
		}

		/* its an int: '[0-9]+' */
		if(c >= '0' && c <= '9') {
			while(1) {
				c = *info->str;
				if(info->tokLen >= MAX_TOKEN_LEN || c == '\0')
					return false;
				if(c < '0' || c > '9')
					break;
				info->token[info->tokLen++] = c;
				info->str++;
			}
			info->token[info->tokLen] = '\0';
			info->tokType = TOK_INT;
			break;
		}

		/* its a string: '"something"' */
		if(c == '"') {
			/* skip '"' */
			info->str++;
			while(1) {
				c = *info->str;
				if(info->tokLen >= MAX_TOKEN_LEN || c == '\0')
					return false;
				if(c == '"') {
					info->str++;
					break;
				}
				info->token[info->tokLen++] = c;
				info->str++;
			}
			info->token[info->tokLen] = '\0';
			info->tokType = TOK_STRING;
			break;
		}

		/* its a name: 'something:' */
		while(1) {
			c = *info->str;
			if(info->tokLen >= MAX_TOKEN_LEN || c == '\0')
				return false;
			/* skip ':' */
			if(c == ':') {
				info->str++;
				break;
			}
			info->token[info->tokLen++] = c;
			info->str++;
		}
		info->token[info->tokLen] = '\0';
		info->tokType = TOK_NAME;
		break;
	}
	return true;
}

static bool app_parseDriverList(sParseInfo *info,sSLList *list) {
	sDriverPerm *drv;
	while(1) {
		if(!app_nextToken(info))
			return false;
		if(info->tokType == TOK_SEM)
			break;
		if(info->tokType == TOK_COMMA && !app_nextToken(info))
			return false;
		drv = malloc(sizeof(sDriverPerm));
		if(!drv)
			return false;
		if(!app_parseDriver(info,drv)) {
			free(drv);
			return false;
		}
		if(!sll_append(list,drv)) {
			free(drv);
			return false;
		}
	}
	return true;
}

static bool app_parseDriver(sParseInfo *info,sDriverPerm *drv) {
	u32 i;
	/* determine group and name */
	if(info->tokType != TOK_STRING)
		return false;
	if(strcmp(info->token,"BINPRIV") == 0)
		drv->group = DRV_GROUP_BINPRIV;
	else if(strcmp(info->token,"BINPUB") == 0)
		drv->group = DRV_GROUP_BINPUB;
	else if(strcmp(info->token,"TXTPRIV") == 0)
		drv->group = DRV_GROUP_TXTPRIV;
	else if(strcmp(info->token,"TXTPUB") == 0)
		drv->group = DRV_GROUP_TXTPUB;
	else {
		drv->group = DRV_GROUP_CUSTOM;
		if(info->tokLen > DRIV_NAME_LEN)
			return false;
		strcpy(drv->name,info->token);
	}

	/* determine permissions */
	for(i = 0; i < 3; i++) {
		/* read ',' and next token */
		if(!app_nextToken(info) || info->tokType != TOK_COMMA || !app_nextToken(info))
			return false;
		switch(i) {
			case 0:
				drv->read = info->tokLen > 0 && info->token[0] == '1';
				break;
			case 1:
				drv->write = info->tokLen > 0 && info->token[0] == '1';
				break;
			case 2:
				drv->ioctrl = info->tokLen > 0 && info->token[0] == '1';
				break;
		}
	}
	return true;
}

static bool app_parseIntList(sParseInfo *info,sSLList *list) {
	while(1) {
		if(!app_nextToken(info))
			return false;
		if(info->tokType == TOK_SEM)
			break;
		if(info->tokType == TOK_COMMA && !app_nextToken(info))
			return false;
		if(!sll_append(list,(void*)(atoi(info->token))))
			return false;
	}
	return true;
}

static bool app_parseStrList(sParseInfo *info,sSLList *list) {
	char *name;
	while(1) {
		if(!app_nextToken(info))
			return false;
		if(info->tokType == TOK_SEM)
			break;
		if(info->tokType == TOK_COMMA && !app_nextToken(info))
			return false;
		name = malloc(info->tokLen + 1);
		if(!name)
			return false;
		strcpy(name,info->token);
		if(!sll_append(list,name)) {
			free(name);
			return false;
		}
	}
	return true;
}

static bool app_parseRangeList(sParseInfo *info,sSLList *list) {
	while(1) {
		if(!app_nextToken(info))
			return false;
		if(info->tokType == TOK_SEM)
			break;
		if(info->tokType == TOK_COMMA && !app_nextToken(info))
			return false;
		if(!app_parseRange(info,list))
			return false;
	}
	return true;
}

static bool app_parseRange(sParseInfo *info,sSLList *list) {
	sRange *r = malloc(sizeof(sRange));
	if(r == NULL)
		return false;
	/* first token already read */
	if(info->tokType != TOK_INT)
		goto error;
	r->start = atoi(info->token);
	if(!app_nextToken(info) || info->tokType != TOK_DOTDOT)
		goto error;
	if(!app_nextToken(info) || info->tokType != TOK_INT)
		goto error;
	r->end = atoi(info->token);
	if(!sll_append(list,r))
		goto error;
	return true;
error:
	free(r);
	return false;
}
