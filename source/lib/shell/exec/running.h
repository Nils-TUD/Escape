/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#ifndef RUNNING_H_
#define RUNNING_H_

#include <esc/common.h>

#define CMD_ID_ALL			0

typedef uint tCmdId;

typedef struct sRunningProc sRunningProc;
struct sRunningProc {
	bool removable;
	tCmdId cmdId;
	pid_t pid;
};

/**
 * Initializes the run-module
 */
void run_init(void);

/**
 * Requests a new command-id
 *
 * @return the command-id
 */
tCmdId run_requestId(void);

/**
 * Adds the given process to the given command
 *
 * @param cmdId the command-id
 * @param pid the pid
 * @return the entry on success or NULL on failure
 */
bool run_addProc(tCmdId cmdId,pid_t pid);

/**
 * Returns the <i>'th running process of the given command
 *
 * @param cmdId the command-id
 * @param i the number
 * @return the process or NULL
 */
sRunningProc *run_getXProcOf(tCmdId cmdId,int i);

/**
 * Searches for the running process with given id
 *
 * @param cmdId the command-id (CMD_ID_ALL to search in all commands)
 * @param pid the process-id
 * @return the information about the process or NULL
 */
sRunningProc *run_findProc(tCmdId cmdId,pid_t pid);

/**
 * Removes and free's all removable processes
 */
void run_gc(void);

/**
 * Removes the process with given id from the running ones
 *
 * @param pid the pid
 */
void run_remProc(pid_t pid);

#endif /* RUNNING_H_ */
