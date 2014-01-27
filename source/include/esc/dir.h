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

#include <esc/common.h>
#include <esc/fsinterface.h>
#include <stdio.h>

typedef FILE DIR;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Builds an absolute path from the given one. If it is not absolute (starts not with "/") CWD will
 * be taken to build the absolute path. The path will end with a slash.
 * If <dst> is not large enough the function stops and returns the number of yet written chars.
 *
 * @param dst where to write to
 * @param dstSize the size of the space <dst> points to
 * @param src your relative path
 * @return the number of written chars (without null-termination)
 */
size_t abspath(char *dst,size_t dstSize,const char *src);

/**
 * Removes the last path-component, if possible
 *
 * @param path the path
 */
void dirname(char *path);

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
 * Reads the next directory-entry from the given file-descriptor.
 *
 * @param e the dir-entry to read into
 * @param dir the dir-pointer
 * @return false if the end has been reached
 */
bool readdir(DIR *dir,sDirEntry *e);

/**
 * Closes the given directory
 *
 * @param dir the dir-pointer
 */
static inline void closedir(DIR *dir) {
	fclose(dir);
}

#ifdef __cplusplus
}
#endif
