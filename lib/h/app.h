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
#include <asprintf.h>

#define APP_TYPE_USER		0
#define APP_TYPE_DRIVER		1
#define APP_TYPE_SERVICE	2
#define APP_TYPE_FS			3

/* information about an application */
typedef struct {
	u8 type;
	char *name;
	char *start;
	char *desc;
} sApp;

/**
 * Writes all info for <app> to the string specified by <str>.
 *
 * @param str the string to write to
 * @param app the application to write
 * @return true if successfull
 */
bool app_toString(sStringBuffer *str,sApp *app);

/**
 * Reads all info specified by <definition> into <app>.
 *
 * @param definition the permissions of the application
 * @param app the app to write to
 * @return the position after parsing in <definition> or NULL if an error occurred
 */
char *app_fromString(const char *definition,sApp *app);

#endif /* APPSPARSER_H_ */
