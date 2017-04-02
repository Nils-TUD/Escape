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
#include <sys/syscalls.h>

#define MAX_PROC_NAME_LEN	30

typedef void (*fExitFunc)(void *arg);

#if defined(__cplusplus)
extern "C" {
#endif

static const uid_t ROOT_UID				= 0;
static const gid_t ROOT_GID				= 0;

extern char **environ;

/**
 * @return the process-id
 */
static inline pid_t getpid(void) {
	return syscall0(SYSCALL_PID);
}

/**
 * @return the parent-pid of the current process
 */
static inline pid_t getppid(void) {
	return syscall0(SYSCALL_PPID);
}

/**
 * @return the user-id of the current process
 */
static inline uid_t getuid(void) {
	return syscall0(SYSCALL_GETUID);
}
static inline uid_t geteuid(void) {
	return getuid();
}

/**
 * Sets the user-id of the current process. This is allowed only if the
 * user-id of the current process is ROOT_UID.
 *
 * @param uid the new uid
 * @return 0 on success
 */
static inline int setuid(uid_t uid) {
	return syscall1(SYSCALL_SETUID,uid);
}
static inline int seteuid(uid_t uid) {
	return setuid(uid);
}

/**
 * @return the group-id of the current process
 */
static inline gid_t getgid(void) {
	return syscall0(SYSCALL_GETGID);
}
static inline gid_t getegid(void) {
	return getgid();
}

/**
 * Sets the group-id of the current process. This is allowed only if the
 * user-id of the current process is ROOT_UID.
 *
 * @param gid the new gid
 * @return 0 on success
 */
static inline int setgid(gid_t gid) {
	return syscall1(SYSCALL_SETGID,gid);
}
static inline int setegid(gid_t gid) {
	return setgid(gid);
}

/**
 * Copies at most <size> groups of the current process into <list>.
 * If <size> is 0, the number of groups is returned.
 *
 * @param size the number of elements in the array <list>
 * @param list the list to copy the group-ids to
 * @return the number of copied group-ids (or the total number of group-ids, if <size> is 0)
 */
static inline int getgroups(size_t size,gid_t *list) {
	return syscall2(SYSCALL_GETGROUPS,size,(ulong)list);
}

/**
 * Sets the groups of the current process to <list>. This is only allowed if the owner of the
 * current process is ROOT_UID.
 *
 * @param size the number of group-ids
 * @param list the group-ids
 * @return 0 on success
 */
static inline int setgroups(size_t size,const gid_t *list) {
	return syscall2(SYSCALL_SETGROUPS,size,(ulong)list);
}

/**
 * Checks whether the process <pid> is in the group <gid>.
 *
 * @param pid the process-id
 * @param gid the group-id
 * @return 1 if so, 0 if not, negative if an error occurred
 */
static inline int isingroup(pid_t pid,gid_t gid) {
	return syscall2(SYSCALL_ISINGROUP,pid,gid);
}

/**
 * Clones the current process
 *
 * @return new pid for parent, 0 for child, < 0 if failed
 */
A_CHECKRET static inline int fork(void) {
	return syscall0(SYSCALL_FORK);
}

/**
 * Exchanges the process-data with the given program
 *
 * @param path the program-path
 * @param args a NULL-terminated array of arguments
 * @return a negative error-code if failed
 */
int execv(const char *path,const char **args);

/**
 * Exchanges the process-data with the given program
 *
 * @param fd the file descriptor to the program (with exec and read permissions; closed on success)
 * @param args a NULL-terminated array of arguments
 * @return a negative error-code if failed
 */
int fexecv(int fd,const char **args);

/**
 * The same as execv(), but if <file> does not contain a slash, the environment variable PATH
 * is prepended to <file>. Afterwards exec() is called with that path and <args>.
 *
 * @param file the file to execute
 * @param args a NULL-terminated array of arguments
 * @return a negative error-code if failed
 */
int execvp(const char *file,const char **args);

/**
 * The same as execv(), but instead of the current environment, <env> is passed to the program.
 *
 * @param path the program-path
 * @param args a NULL-terminated array of arguments
 * @param env a NULL-terminated array of environment-variables
 * @return a negative error-code if failed
 */
int execvpe(const char *path,const char **args,const char **env);

/**
 * The same as fexecv(), but instead of the current environment, <env> is passed to the program.
 *
 * @param fd the file descriptor to the program (with exec and read permissions; closed on success)
 * @param args a NULL-terminated array of arguments
 * @param env a NULL-terminated array of environment-variables
 * @return a negative error-code if failed
 */
int fexecvpe(int fd,const char **args,const char **env);

/**
 * The system function is used to issue a command. Execution of your program will not
 * continue until the command has completed.
 *
 * @param cmd string containing the system command to be executed.
 * @return The value returned when the argument passed is not NULL, depends on the running
 * 	environment specifications. In many systems, 0 is used to indicate that the command was
 * 	successfully executed and other values to indicate some sort of error.
 * 	When the argument passed is NULL, the function returns a nonzero value if the command
 * 	processor is available, and zero otherwise.
 */
int system(const char *cmd);

/**
 * Registers the given function for calling when _exit() is called. Registered functions are
 * called in reverse order. Note that the number of functions is limited!
 *
 * @param func the function
 * @return 0 if successfull
 */
int atexit(fExitFunc func);

/**
 * Calls atexit-functions and then _exit()
 *
 * @param errorCode the error-code for the parent
 */
void exit(int errorCode) A_NORETURN;

#if defined(__cplusplus)
}
#endif
