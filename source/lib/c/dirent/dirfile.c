/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <dirent.h>
#include <string.h>

const char *dirfile(char *path,char **filename) {
	/* first, remove potential slashes at the end */
	size_t len = strlen(path);
	while(len > 0 && path[len - 1] == '/')
		path[--len] = '\0';

	/* now, walk to the previous '/' */
	while(len > 0 && path[len - 1] != '/')
		len--;

	/* if there is none, the filename is the path */
	if(len == 0) {
		*filename = path;
		return ".";
	}

	/* if there is, split at that point */
	*filename = path + len;
	path[len - 1] = '\0';
	if(*path)
		return path;
	return "/";
}
