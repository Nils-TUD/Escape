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

#include <sys/common.h>
#include <sys/io.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

int execv(const char *path,const char **args) {
	char apath[MAX_PATH_LEN];
	return syscall3(SYSCALL_EXEC,(ulong)abspath(apath,sizeof(apath),path),(ulong)args,(ulong)environ);
}

int execvpe(const char *path,const char **args,const char **env) {
	char apath[MAX_PATH_LEN];
	return syscall3(SYSCALL_EXEC,(ulong)abspath(apath,sizeof(apath),path),(ulong)args,(ulong)env);
}

int execvp(const char *file,const char **args) {
	char path[MAX_PATH_LEN];
	size_t len,flen;

	/* if there is a slash in file, use the default exec */
	if(strchr(file,'/') != NULL)
		return execv(file,args);

	/* get path */
	if(getenvto(path,sizeof(path),"PATH") < 0)
		return execv(file,args);

	/* append file */
	len = strlen(path);
	if(len < MAX_PATH_LEN - 1 && path[len - 1] != '/') {
		path[len++] = '/';
		path[len] = '\0';
	}
	flen = strlen(file);
	if(len + flen < MAX_PATH_LEN)
		strcpy(path + len,file);

	/* exec with that path */
	return execv(path,args);
}
