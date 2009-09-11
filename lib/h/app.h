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

#ifndef APPSPARSER_H_
#define APPSPARSER_H_

#include <types.h>
#include <sllist.h>
#include <asprintf.h>

#define DRIV_NAME_LEN		15

#define APP_TYPE_DRIVER		0
#define APP_TYPE_SERVICE	1
#define APP_TYPE_FS			2
#define APP_TYPE_DEFAULT	3

#define DRIVER_PERM_GROUP	0
#define DRIVER_PERM_NAME	1

#define DRV_GROUP_CUSTOM	0
#define DRV_GROUP_BINPRIV	1
#define DRV_GROUP_BINPUB	2
#define DRV_GROUP_TXTPRIV	3
#define DRV_GROUP_TXTPUB	4

typedef struct {
	u32 start;
	u32 end;
} sRange;

typedef struct {
	u8 group;
	char name[DRIV_NAME_LEN + 1];
	bool read;
	bool write;
	bool ioctrl;
} sDriverPerm;

typedef struct {
	u32 id;
	void *db;
	u16 appType;			/* driver, fs, service or default */
	sSLList *ioports;		/* list of sRange */
	sSLList *driver;		/* list of sDriverPerm */
	struct {
		bool read;
		bool write;
	} fs;
	sSLList *services;		/* list of names */
	sSLList *signals;		/* list of signal-numbers */
	sSLList *physMem;		/* list of sRange */
	sSLList *createShMem;	/* list of names */
	sSLList *joinShMem;		/* list of names */
} sApp;

/* format:
		source:							"name";
		sourceWritable:					1|0;
		type:							"driver|fs|service|default";
		ioports:						X1..Y1,X2..Y2,...,Xn..Yn;
		driver:
			"group|name",1|0,1|0,1|0,
			...,
			"group|name",1|0,1|0,1|0;
		fs:								1|0,1|0;
		services:						"name1","name2",...,"namen";
		signals:						sig1,sig2,...,sign;
		physmem:						X1..Y1,X2..Y2,...,Xn..Yn;
		crtshmem:						"name1","name2",...,"namen";
		joinshmem:						"name1","name2",...,"namen";
*/

bool app_toString(sStringBuffer *str,sApp *app,char *src,bool srcWritable);

bool app_parse(const char *definition,sApp *app,char *src,bool *srcWritable);

#endif /* APPSPARSER_H_ */
