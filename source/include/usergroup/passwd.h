/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#ifndef PASSWD_H_
#define PASSWD_H_

#include <esc/common.h>
#include <stdio.h>

#define PASSWD_PATH			"/etc/passwd"
#define MAX_PW_LEN			31

typedef struct sPasswd {
	uid_t uid;
	char pw[MAX_PW_LEN + 1];
	struct sPasswd *next;
} sPasswd;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parses the password-information from given file into a linked list of passwords.
 * Please free the memory with pw_free() if you're done.
 *
 * @param file the file to read
 * @param count will be set to the number of passwords in the returned list (if not NULL)
 * @return the passwords or NULL if failed
 */
sPasswd *pw_parseFromFile(const char *file,size_t *count);

/**
 * Parses the given password-information into a linked list of passwords.
 * Please free the memory with pw_free() if you're done.
 *
 * @param pws the password-information (null-terminated)
 * @param count will be set to the number of passwords in the returned list (if not NULL)
 * @return the passwords or NULL if failed
 */
sPasswd *pw_parse(const char *pws,size_t *count);

/**
 * Appends the given password to <list>
 *
 * @param list the password-list
 * @param pw the password
 */
void pw_append(sPasswd *list,sPasswd *pw);

/**
 * Removes the given password from the list
 *
 * @param list the password-list
 * @param pw the password
 */
void pw_remove(sPasswd *list,sPasswd *pw);

/**
 * Searches for the user with given id
 *
 * @param pws the password-list
 * @param uid the user-id
 * @return the user or NULL
 */
sPasswd *pw_getById(const sPasswd *pws,uid_t uid);

/**
 * Writes the given passwords to the file with given path
 *
 * @param pws the passwords
 * @param path the path
 * @return 0 on success
 */
int pw_writeToFile(const sPasswd *pws,const char *path);

/**
 * Writes the given passwords to the given file
 *
 * @param pws the passwords
 * @param f the file
 * @return 0 on success
 */
int pw_write(const sPasswd *pws,FILE *f);

/**
 * Free's the given passwords
 *
 * @param pws the passwords
 */
void pw_free(sPasswd *pws);

/**
 * Prints the given passwords
 *
 * @param pws the passwords
 */
void pw_print(const sPasswd *pws);

#ifdef __cplusplus
}
#endif

#endif /* PASSWD_H_ */
