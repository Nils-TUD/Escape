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

#define CMD_ID_ALL			0

typedef uint tJobId;

typedef struct {
	bool background;
	bool removable;
	FILE *cmd;
	char *command;
	tJobId jobId;
	pid_t pid;
} sJob;

/**
 * Initializes the jobs-module
 */
void jobs_init(void);

/**
 * Requests a new job-id
 *
 * @return the job-id
 */
tJobId jobs_requestId(void);

/**
 * Adds the given process to the given job
 *
 * @param jobId the job-id
 * @param pid the pid
 * @param argc argument count
 * @param argv the arguments
 * @param background whether it runs in background
 * @return true on success or false on failure
 */
bool jobs_addProc(tJobId jobId,pid_t pid,int argc,char **argv,bool background);

/**
 * Returns the <i>'th running process of the given job
 *
 * @param jobId the job-id
 * @param i the number
 * @return the process or NULL
 */
sJob *jobs_getXProcOf(tJobId jobId,int i);

/**
 * Searches for the running process with given id
 *
 * @param jobId the job-id (CMD_ID_ALL to search in all job)
 * @param pid the process-id
 * @return the information about the process or NULL
 */
sJob *jobs_findProc(tJobId jobId,pid_t pid);

/**
 * Removes and free's all removable jobs
 */
void jobs_gc(void);

/**
 * Prints all jobs to stdout
 */
void jobs_print(void);

/**
 * Removes the process with given id from the running ones
 *
 * @param pid the pid
 * @return true on success
 */
bool jobs_remProc(pid_t pid);
