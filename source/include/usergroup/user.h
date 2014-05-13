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
#include <stdio.h>

#define USERS_PATH			"/etc/users"
#define MAX_USERNAME_LEN	31

typedef struct sUser {
	uid_t uid;
	gid_t gid;
	char name[MAX_USERNAME_LEN + 1];
	char home[MAX_PATH_LEN + 1];
	struct sUser *next;
} sUser;

#if defined(__cplusplus)
extern "C" {
#endif

typedef void *(*parse_func)(const char *buffer,size_t *count);

/**
 * Internal function to parse a list of things from a file.
 *
 * @param file the file to read
 * @param count will be set to the number of items in the returned list
 * @param parse the parse function
 * @return the list
 */
void *user_parseListFromFile(const char *file,size_t *count,parse_func parse);

/**
 * Internal function to read a string into a buffer.
 *
 * @param dst the buffer to write into
 * @param src the buffer to read from
 * @param max the space in <dst>
 * @param last whether it's the last one
 * @return the new position in <src>
 */
const char *user_readStr(char *dst,const char *src,size_t max,bool last);

/**
 * Parses the user-information from given file into a linked list of users.
 * Please free the memory with user_free() if you're done.
 *
 * @param file the file to read
 * @param count will be set to the number of users in the returned list (if not NULL)
 * @return the users or NULL if failed
 */
sUser *user_parseFromFile(const char *file,size_t *count);

/**
 * Parses the given user-information into a linked list of users.
 * Please free the memory with user_free() if you're done.
 *
 * @param users the user-information (null-terminated)
 * @param count will be set to the number of users in the returned list (if not NULL)
 * @return the users or NULL if failed
 */
sUser *user_parse(const char *users,size_t *count);

/**
 * Appends the given user to <list>
 *
 * @param list the user-list
 * @param u the user
 */
void user_append(sUser *list,sUser *u);

/**
 * Removes the given user from the list
 *
 * @param list the user-list
 * @param u the user
 */
void user_remove(sUser *list,sUser *u);

/**
 * Searches for a free user-id in the given list
 *
 * @param u the user-list
 * @return the free user-id
 */
uid_t user_getFreeUid(const sUser *u);

/**
 * Searches for the user with given name
 *
 * @param u the user-list
 * @param name the user-name
 * @return the user or NULL
 */
sUser *user_getByName(const sUser *u,const char *name);

/**
 * Searches for the user with given id
 *
 * @param u the user-list
 * @param uid the user-id
 * @return the user or NULL
 */
sUser *user_getById(const sUser *u,uid_t uid);

/**
 * Writes the given users to the file with given path
 *
 * @param u the users
 * @param path the path
 * @return 0 on success
 */
int user_writeToFile(const sUser *u,const char *path);

/**
 * Writes the given users to the given file
 *
 * @param u the users
 * @param f the file
 * @return 0 on success
 */
int user_write(const sUser *u,FILE *f);

/**
 * Free's the given users
 *
 * @param u the users
 */
void user_free(sUser *u);

/**
 * Prints the given users
 *
 * @param u the users
 */
void user_print(const sUser *u);

#if defined(__cplusplus)
}
#endif
