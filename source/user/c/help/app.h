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

#ifndef APPSPARSER_H_
#define APPSPARSER_H_

#include <esc/common.h>

#define APP_TYPE_USER		0
#define APP_TYPE_DEVICE		1
#define APP_TYPE_FS			2

/* information about an application */
typedef struct {
	uint type;
	char *name;
	char *start;
	char *desc;
} sApp;

/**
 * Reads all info specified by <definition> into <app>.
 *
 * @param definition the permissions of the application
 * @param app the app to write to
 * @return the position after parsing in <definition> or NULL if an error occurred
 */
char *app_fromString(const char *definition,sApp *app);

#endif /* APPSPARSER_H_ */
