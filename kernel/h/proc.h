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

#include "common.h"
#include "intrpt.h"
#include "fpu.h"
#include "text.h"
#include "vfsnode.h"

/* max number of processes */
#define PROC_COUNT			1024
#define MAX_PROC_NAME_LEN	30

/* for marking unused */
#define INVALID_PID			(PROC_COUNT + 2)

/* the events we can wait for */
#define EV_NOEVENT			0
#define EV_CLIENT			1
#define EV_RECEIVED_MSG		2
#define EV_CHILD_DIED		4
#define EV_DATA_READABLE	8
#define EV_UNLOCK			16	/* kernel-intern */

/* represents a process */
/* TODO move stuff for existing processes to the kernel-stack-page */
typedef struct {
	/* process id (2^16 processes should be enough :)) */
	tPid pid;
	/* parent process id */
	tPid parentPid;
	/* the physical address for the page-directory of this process */
	u32 physPDirAddr;
	/* the text of this process. NULL if it has no text */
	sTextUsage *text;
	/* the number of pages per segment */
	u32 textPages;
	u32 dataPages;
	u32 stackPages;
	/* the io-map (NULL by default) */
	u8 *ioMap;
	/* start-command */
	char command[MAX_PROC_NAME_LEN + 1];
	/* threads of this process */
	sSLList *threads;
	/* the directory-node in the VFS of this process */
	sVFSNode *threadDir;
} sProc;

/* the area for proc_changeSize() */
typedef enum {CHG_DATA,CHG_STACK} eChgArea;

/**
 * Initializes the process-management
 */
void proc_init(void);

/**
 * Searches for a free pid and returns it or 0 if there is no free process-slot
 *
 * @return the pid or 0
 */
tPid proc_getFreePid(void);

/**
 * @return the running process
 */
sProc *proc_getRunning(void);

/**
 * @param pid the pid of the process
 * @return the process with given pid
 */
sProc *proc_getByPid(tPid pid);

/**
 * Checks wether the process with given id exists
 *
 * @param pid the process-id
 * @return true if so
 */
bool proc_exists(tPid pid);

/**
 * @return the number of existing processes
 */
u32 proc_getCount(void);

/**
 * Determines wether the given process has a child
 *
 * @param pid the process-id
 * @return true if it has a child
 */
bool proc_hasChild(tPid pid);

/**
 * Destroys zombies
 */
void proc_cleanup(void);

/**
 * Clones the current process into the given one, gives the new process a clone of the current
 * thread and saves this thread in proc_clone() so that it will start there on thread_resume().
 * The function returns -1 if there is not enough memory.
 *
 * @param newPid the target-pid
 * @return -1 if an error occurred, 0 for parent, 1 for child
 */
s32 proc_clone(tPid newPid);

/**
 * Starts a new thread at given entry-point. Will clone the kernel-stack from the current thread
 *
 * @return < 0 if an error occurred, new tid for current thread, 0 for new thread
 */
s32 proc_startThread(u32 entryPoint);

/**
 * Destroys the current thread. If it's the only thread in the process, the complete process will
 * destroyed.
 * Note that the actual deletion will be done later!
 */
void proc_destroyThread(void);

/**
 * Destroyes the given process. That means the process-slot will be marked as "unused" and the
 * paging-structure will be freed.
 * Note that the actual deletion will be done later if p is the current process!
 *
 * @param p the process
 */
void proc_destroy(sProc *p);

/**
 * Setups the user-stack for given interrupt-stack of the current process
 *
 * @param frame the interrupt-stack-frame
 * @param argc the argument-count
 * @param args the arguments on after another, allocated on the heap; may be NULL
 * @param argsSize the total number of bytes for the arguments (just the data)
 */
void proc_setupUserStack(sIntrptStackFrame *frame,u32 argc,char *args,u32 argsSize);

/**
 * Setups the start of execution in user-mode for given interrupt-stack
 *
 * @param frame the interrupt-stack-frame
 * @param entryPoint the entry-point
 */
void proc_setupStart(sIntrptStackFrame *frame,u32 entryPoint);

/**
 * Checks wether the given segment-sizes are valid
 *
 * @param textPages the number of text-pages
 * @param dataPages the number of data-pages
 * @param stackPages the number of stack-pages
 * @return true if so
 */
bool proc_segSizesValid(u32 textPages,u32 dataPages,u32 stackPages);

/**
 * Changes the size of either the data-segment of the current process or the stack-segment of
 * the current thread.
 * If <change> is positive pages will be added and otherwise removed. Added pages
 * will always be cleared.
 * If there is not enough memory the function returns false.
 * Note that the size of the current process (dataPages / stackPages) will be adjusted!
 *
 * @param change the number of pages to add or remove
 * @param area the data or stack? (CHG_*)
 * @return true if successfull
 */
bool proc_changeSize(s32 change,eChgArea area);

#if DEBUGGING

/**
 * Prints all existing processes
 */
void proc_dbg_printAll(void);

/**
 * Prints the given process
 *
 * @param p the pointer to the process
 */
void proc_dbg_print(sProc *p);

#endif

#endif /*PROC_H_*/
