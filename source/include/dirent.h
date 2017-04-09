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

#pragma once

#include <sys/common.h>
#include <stdio.h>

#define NAME_MAX		52

/* a directory-entry */
struct dirent {
	ino_t d_ino;
	uint16_t d_reclen;
	uint16_t d_namelen;
	char d_name[NAME_MAX + 1];
} A_PACKED;

typedef FILE DIR;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Builds a canonical path from the given one, i.e., resolves all symlinks, makes it absolute and
 * removes duplicate slashes, "." and "..".
 *
 * @param dst where to write to
 * @param dstSize the size of the space <dst> points to
 * @param src your relative path
 * @return the number of written chars (without null-termination) or a negative error code
 */
ssize_t canonpath(char *dst,size_t dstSize,const char *src);

/**
 * Removes duplicate slashes, "." and "..". Assumes that the path contains no symbol links.
 *
 * @param dst where to write to
 * @param dstSize the size of the space <dst> points to
 * @param src your path
 * @return the number of written chars (without null-termination)
 */
size_t cleanpath(char *dst,size_t dstSize,const char *src);

/**
 * Builds an absolute path from <path>, if necessary.
 *
 * @param path the path
 * @return the absolute path (might use <path> or a statically allocated array)
 */
char *abspath(char *dst,size_t dstSize,const char *path);

/**
 * Returns the base-name of the given path, i.e. the last path-component. It will not contain a
 * slash, which is why <path> might get changed.
 *
 * @param path the path to change
 * @return the base-name
 */
char *basename(char *path);

/**
 * Returns the directory-name of the given path, i.e. the second last path-component. It might
 * change <path>.
 *
 * @param path the path to change
 * @return the directory-name
 */
char *dirname(char *path);

/**
 * Splits <path> to retrieve the directory name and the filename.
 *
 * @param path the path
 * @param filename will point to the filename
 * @return the directory name
 */
const char *dirfile(char *path,char **filename);

/**
 * Opens the given directory
 *
 * @param path the path to the directory
 * @return the dir-pointer or NULL if it failed
 */
static inline DIR *opendir(const char *path) {
	return (DIR*)fopen(path,"r");
}

/**
 * Returns the next directory-entry from the given directory pointer.
 *
 * @param dir the dir-pointer
 * @return the directory entry (statically allocated)
 */
struct dirent *readdir(DIR *dir);

/**
 * Stores the next directory-entry from the given directory pointer into <e>.
 *
 * @param e the dir-entry to read into
 * @param dir the dir-pointer
 * @return false if the end has been reached
 */
bool readdirto(DIR *dir,struct dirent *e);

/**
 * Closes the given directory
 *
 * @param dir the dir-pointer
 * @return 0 on success
 */
static inline int closedir(DIR *dir) {
	fclose(dir);
	return 0;
}

#if defined(__cplusplus)
}
#endif
