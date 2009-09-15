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
#define APP_NAME_LEN		30

/* driver-operations */
#define DRV_OP_READ			1
#define DRV_OP_WRITE		2
#define DRV_OP_IOCTL		4

/* fs-operations */
#define FS_OP_READ			1
#define FS_OP_WRITE			2

/* possible app-types */
#define APP_TYPE_DRIVER		0
#define APP_TYPE_SERVICE	1
#define APP_TYPE_FS			2
#define APP_TYPE_DEFAULT	3

/* possible driver-groups */
#define DRV_GROUP_CUSTOM	0
#define DRV_GROUP_BINPRIV	1
#define DRV_GROUP_BINPUB	2
#define DRV_GROUP_TXTPRIV	3
#define DRV_GROUP_TXTPUB	4

/* some kind of range */
typedef struct {
	u32 start;
	u32 end;
} sRange;

/* permissions for a driver */
typedef struct {
	u8 group;
	char name[DRIV_NAME_LEN + 1];
	bool read;
	bool write;
	bool ioctrl;
} sDriverPerm;

/* information about an application */
typedef struct {
	void *db;
	char name[APP_NAME_LEN + 1];
	u16 appType;			/* driver, fs, service or default */
	sSLList *ioports;		/* list of sRange */
	sSLList *driver;		/* list of sDriverPerm */
	struct {
		bool read;
		bool write;
	} fs;
	sSLList *services;		/* list of names */
	sSLList *intrpts;		/* list of signal-numbers */
	sSLList *physMem;		/* list of sRange */
	sSLList *createShMem;	/* list of names */
	sSLList *joinShMem;		/* list of names */
} sApp;

/**
 * Writes all settings for <app> to the string specified by <str>.
 *
 * @param str the string to write to
 * @param app the application to write
 * @parma src the source of the application
 * @param srcWritable wether the source is writable
 * @return true if successfull
 */
bool app_toString(sStringBuffer *str,sApp *app,char *src,bool srcWritable);

/**
 * Reads all settings specified by <definition> into <app>, <src> and <srcWritable>.
 *
 * @param definition the permissions of the application
 * @param app the app to write to
 * @param src the source of the app
 * @param srcWritable wether the source is writable
 * @return the position after parsing in <definition> or NULL if an error occurred
 */
char *app_fromString(const char *definition,sApp *app,char *src,bool *srcWritable);

#endif /* APPSPARSER_H_ */
