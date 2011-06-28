/**
 * $Id$
 */

#ifndef USER_H_
#define USER_H_

#include <esc/common.h>
#include <stdio.h>

#define USERS_PATH			"/etc/users"
#define MAX_USERNAME_LEN	32
#define MAX_PW_LEN			32

typedef struct sUser {
	uid_t uid;
	gid_t gid;
	char name[MAX_USERNAME_LEN];
	char pw[MAX_PW_LEN];
	char home[MAX_PATH_LEN];
	struct sUser *next;
} sUser;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parses the user-information from given file into a linked list of users.
 * Please free the memory with user_free() if you're done.
 *
 * @param file the file to read
 * @param count will be set to the number of users in the returned list
 * @return the users or NULL if failed
 */
sUser *user_parseFromFile(const char *file,size_t *count);

/**
 * Parses the given user-information into a linked list of users.
 * Please free the memory with user_free() if you're done.
 *
 * @param users the user-information (null-terminated)
 * @param count will be set to the number of users in the returned list
 * @return the users or NULL if failed
 */
sUser *user_parse(const char *users,size_t *count);

/**
 * Searches for the user with given id
 *
 * @param u the user-list
 * @param uid the user-id
 * @return the user or NULL
 */
sUser *user_getById(sUser *u,uid_t uid);

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

#ifdef __cplusplus
}
#endif

#endif /* USER_H_ */
