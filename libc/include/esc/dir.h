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

#ifndef DIR_H_
#define DIR_H_

#include <esc/common.h>
#include <esc/io.h>

#define MAX_NAME_LEN			50
#define MAX_PATH_LEN			255

/* a directory-entry */
typedef struct {
	tVFSNodeNo nodeNo;
	u16 recLen;
	u16 nameLen;
	char name[];
} __attribute__((packed)) sDirEntry;

/**
 * Builds an absolute path from the given one. If it starts with no namespace ("file:" e.g.)
 * and is not absolute (starts with "/") CWD will be taken to build the absolute path.
 *
 * @param path your relative path
 * @return the absolute path (statically stored!)
 */
char *abspath(const char *path);

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
 * @return the file-descriptor for the directory or a negative error-code
 */
tFD opendir(const char *path);

/**
 * Reads the next directory-entry from the given file-descriptor.
 * Note that the data of the entry might be overwritten by the next call of readdir()!
 *
 * @param dir the file-descriptor
 * @return a pointer to the directory-entry or NULL if the end has been reached
 */
sDirEntry *readdir(tFD dir);

/**
 * Closes the given directory
 *
 * @param dir the file-descriptor
 */
void closedir(tFD dir);


#endif /* DIR_H_ */
