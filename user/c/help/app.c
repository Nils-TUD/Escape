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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "app.h"

#define MAX_TOKEN_LEN	1024

#define TOK_STRING		0
#define TOK_SEM			1
#define TOK_NAME		2
#define TOK_EOF			3

typedef struct {
	const char *str;
	u32 tokLen;
	u32 tokType;
	char token[MAX_TOKEN_LEN + 1];
} sParseInfo;

static bool app_nextToken(sParseInfo *info);

char *app_fromString(const char *definition,sApp *app) {
	sParseInfo info;
	info.str = definition;
	app->name = NULL;
	app->desc = NULL;
	app->start = NULL;
	app->type = APP_TYPE_USER;
	while(1) {
		if(!app_nextToken(&info)) {
			if(info.tokType == TOK_EOF)
				break;
			goto error;
		}
		if(info.tokType != TOK_NAME)
			goto error;

		if(strcmp(info.token,"name") == 0) {
			if(!app_nextToken(&info) || info.tokType != TOK_STRING)
				goto error;
			app->name = (char*)malloc(info.tokLen + 1);
			if(!app->name)
				goto error;
			strcpy(app->name,info.token);
		}
		else if(strcmp(info.token,"desc") == 0) {
			if(!app_nextToken(&info) || info.tokType != TOK_STRING)
				goto error;
			app->desc = (char*)malloc(info.tokLen + 1);
			if(!app->desc)
				goto error;
			strcpy(app->desc,info.token);
		}
		else if(strcmp(info.token,"start") == 0) {
			if(!app_nextToken(&info) || info.tokType != TOK_STRING)
				goto error;
			app->start = (char*)malloc(info.tokLen + 1);
			if(!app->start)
				goto error;
			strcpy(app->start,info.token);
		}
		else if(strcmp(info.token,"type") == 0) {
			if(!app_nextToken(&info) || info.tokType != TOK_STRING)
				goto error;
			if(strcmp(info.token,"user") == 0)
				app->type = APP_TYPE_USER;
			else if(strcmp(info.token,"driver") == 0)
				app->type = APP_TYPE_DRIVER;
			else if(strcmp(info.token,"fs") == 0)
				app->type = APP_TYPE_FS;
			else
				goto error;
		}
		else
			goto error;
	}

	/* do we have a valid app? */
	if(app->name != NULL && app->desc != NULL && app->start != NULL)
		return (char*)info.str;

error:
	free(app->name);
	app->name = NULL;
	free(app->desc);
	app->desc = NULL;
	free(app->start);
	app->start = NULL;
	return NULL;
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
