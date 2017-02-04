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

#if defined(__cplusplus)
extern "C" {
#endif

#define USERS_PATH			"/etc/users"
#define GROUPS_PATH			"/etc/groups"

#define MAX_NAME_LEN		31
#define MAX_PW_LEN			31

/* fixed because boot-modules can't use the fs and thus, can't read /etc/groups */
#define USER_BUS			1
#define USER_STORAGE		2
#define USER_FS				3

#define GROUP_DRIVER		1
#define GROUP_BUS			2
#define GROUP_STORAGE		3
#define GROUP_FS			4

typedef struct sNamedItem {
	int id;
	char name[MAX_NAME_LEN + 1];
	struct sNamedItem *next;
} sNamedItem;

/**
 * Reads the given directory and builds a list of items from it.
 * Please free the memory with usergroup_free() if you're done.
 *
 * @param count will be set to the number of items in the returned list (if not NULL)
 * @return the items or NULL if failed
 */
sNamedItem *usergroup_parse(const char *path,size_t *count);

/**
 * Searches for a free id in the given list
 *
 * @param i the list
 * @return the free id
 */
int usergroup_getFreeId(const sNamedItem *i);

/**
 * Searches for the item with given name
 *
 * @param i the list
 * @param name the name
 * @return the item or NULL
 */
sNamedItem *usergroup_getByName(const sNamedItem *i,const char *name);

/**
 * Searches for the item with given id
 *
 * @param i the list
 * @param id the id
 * @return the item or NULL
 */
sNamedItem *usergroup_getById(const sNamedItem *i,int id);

/**
 * Checks whether the given group id is in use
 *
 * @param gid the group id
 * @return true if so
 */
bool usergroup_groupInUse(gid_t gid);

 /**
 * Collects all groups for the given user
 *
 * @param user the user
 * @param openSlots specifies how many slots should be left open in the groups-array
 * @param count will be set to the number of collected groups
 * @return the group-id-array or NULL if failed
 */
gid_t *usergroup_collectGroupsFor(const char *user,size_t openSlots,size_t *count);

/**
 * Change the user and groups of the process to user with the id <uid>.
 *
 * @param uid the user id
 * @return 0 on success
 */
int usergroup_changeToId(uid_t uid);

/**
 * Change the user and groups of the process to user with the name <name>.
 *
 * @param name the user name
 * @return 0 on success
 */
int usergroup_changeToName(const char *name);

/**
 * Free's the given list
 *
 * @param i the list
 */
void usergroup_free(sNamedItem *i);

/**
 * Prints the given list
 *
 * @param i the list
 */
void usergroup_print(const sNamedItem *i);

/**
 * Loads the given integer attribute.
 *
 * @param folder the folder to start at
 * @param name the user/group name
 * @param attr the attribute name
 * @return the integer or a negative error code
 */
int usergroup_loadIntAttr(const char *folder,const char *name,const char *attr);

/**
 * Stores the given integer attribute.
 *
 * @param folder the folder to start at
 * @param name the user/group name
 * @param attr the attribute name
 * @param value the value to store
 * @param mode the mode for the file to set on creation
 * @return 0 on success
 */
int usergroup_storeIntAttr(const char *folder,const char *name,const char *attr,int value,
	mode_t mode);

/**
 * Loads the given string attribute.
 *
 * @param folder the folder to start at
 * @param name the user/group name
 * @param attr the attribute name
 * @param buf the buffer to write to
 * @param size the size of the buffer
 * @return 0 on success
 */
int usergroup_loadStrAttr(const char *folder,const char *name,const char *attr,char *buf,size_t size);

/**
 * Stores the given string attribute.
 *
 * @param folder the folder to start at
 * @param name the user/group name
 * @param attr the attribute name
 * @param str the string to store
 * @param mode the mode for the file to set on creation
 * @return 0 on success
 */
int usergroup_storeStrAttr(const char *folder,const char *name,const char *attr,const char *str,
	mode_t mode);

/**
 * Get the password for the given user
 *
 * @param name the user name
 * @return the password (statically allocated) or NULL
 */
static inline const char *usergroup_getPW(const char *name) {
	static char pw[MAX_PW_LEN + 1];
	if(usergroup_loadStrAttr(USERS_PATH,name,"passwd",pw,sizeof(pw)) < 0)
		return NULL;
	return pw;
}

/**
 * Get the default group for the given user
 *
 * @param name the user name
 * @return the group id on success
 */
static inline int usergroup_getGid(const char *name) {
	return usergroup_loadIntAttr(USERS_PATH,name,"gid");
}

/**
 * Get the home directory for the given user
 *
 * @param name the user name
 * @param buf the buffer to write to
 * @param size the size of the buffer
 * @return 0 on success
 */
static inline int usergroup_getHome(const char *name,char *buf,size_t size) {
	return usergroup_loadStrAttr(USERS_PATH,name,"home",buf,size);
}

/**
 * Determines the name for the given user/group id
 *
 * @param folder the folder to start at
 * @param id the user/group id
 * @return the name or NULL if not found
 */
const char *usergroup_idToName(const char *folder,int id);

/**
 * Determines the id for the given user/group id
 *
 * @param folder the folder to start at
 * @param name the user/group name
 * @return the id or a negative error code
 */
static inline int usergroup_nameToId(const char *folder,const char *name) {
	return usergroup_loadIntAttr(folder,name,"id");
}

#if defined(__cplusplus)
}
#endif
