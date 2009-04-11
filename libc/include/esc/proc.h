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

#ifndef PROC_H_
#define PROC_H_

#include <esc/common.h>

#define MAX_PROC_NAME_LEN	30
#define INVALID_PID			1026

/* the events we can wait for */
#define EV_NOEVENT			0
#define EV_CLIENT			1
#define EV_RECEIVED_MSG		2
#define EV_CHILD_DIED		4

/**
 * @return the process-id
 */
tPid getpid(void);

/**
 * @return the parent-pid of the current process
 */
tPid getppid(void);

/**
 * Returns the parent-id of the given process
 *
 * @param pid the process-id
 * @return the parent-pid
 */
s32 getppidof(tPid pid);

/**
 * Clones the current process
 *
 * @return new pid for parent, 0 for child, < 0 if failed
 */
s32 fork(void);

/**
 * Exchanges the process-data with the given program
 *
 * @param path the program-path
 * @param args a NULL-terminated array of arguments
 * @return a negative error-code if failed
 */
s32 exec(const char *path,const char **args);

/**
 * Releases the CPU (reschedule)
 */
void yield(void);

/**
 * Puts the process to sleep for <msecs> milliseconds.
 *
 * @param msecs the number of milliseconds to wait
 * @return 0 on success
 */
s32 sleep(u32 msecs);

/**
 * Puts the process to sleep until one of the given events occurrs. Note that you will
 * always be waked up for signals!
 *
 * @param events the events on which you want to wake up
 * @return a negative error-code if failed
 */
s32 wait(u8 events);

/**
 * Destroys the process and provides the parent the given error-code
 *
 * @param errorCode the error-code for the parent
 */
void exit(s32 errorCode);

#endif /* PROC_H_ */
