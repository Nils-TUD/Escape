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

#include <sys/common.h>
#include <sys/machine/intrpt.h>
#include <sys/machine/fpu.h>
#include <sys/machine/vm86.h>
#include <sys/mem/region.h>
#include <sys/task/elf.h>
#include <sys/vfs/node.h>

/* max number of coexistent processes */
#define MAX_PROC_COUNT		8192
#define MAX_PROC_NAME_LEN	30
#define MAX_FD_COUNT		64

/* for marking unused */
#define INVALID_PID			(MAX_PROC_COUNT + 1)
#define KERNEL_PID			MAX_PROC_COUNT
#define ATA_PID				2
#define FS_PID				4
#define KEYBOARD_PID		12	/* just for debugging */

/* process flags */
#define P_ZOMBIE			1
#define P_VM86				2

typedef struct {
	tPid pid;
	/* the signal that killed the process (SIG_COUNT if none) */
	tSig signal;
	/* exit-code the process gave us via exit() */
	s32 exitCode;
	/* memory it has used */
	u32 ownFrames;
	u32 sharedFrames;
	u32 swapped;
	/* cycle-count */
	uLongLong ucycleCount;
	uLongLong kcycleCount;
	/* other stats */
	u32 schedCount;
	u32 syscalls;
} sExitState;

/* represents a process */
typedef struct {
	/* flags for vm86 and zombie */
	u8 flags;
	/* process id (2^16 processes should be enough :)) */
	tPid pid;
	/* parent process id */
	tPid parentPid;
	/* the physical address for the page-directory of this process */
	tPageDir pagedir;
	/* the number of frames the process owns, i.e. no cow, no shared stuff, no mapPhysical.
	 * paging-structures are counted, too */
	u32 ownFrames;
	/* the number of frames the process uses, but maybe other processes as well */
	u32 sharedFrames;
	/* pages that are in swap */
	u32 swapped;
	/* the regions */
	u32 regSize;
	void *regions;
	/* the entrypoint of the binary */
	u32 entryPoint;
	/* file descriptors: indices of the global file table */
	tFileNo fileDescs[MAX_FD_COUNT];
	/* channels to send/receive messages to/from fs (needed in vfs/real.c) */
	sSLList *fsChans;
	/* environment-variables of this process */
	sSLList *env;
	/* for the waiting parent */
	sExitState *exitState;
	/* the address of the sigRet "function" */
	u32 sigRetAddr;
	/* the io-map (NULL by default) */
	u8 *ioMap;
	/* start-command */
	char command[MAX_PROC_NAME_LEN + 1];
	/* threads of this process */
	sSLList *threads;
	/* the directory-node in the VFS of this process */
	sVFSNode *threadDir;
	struct {
		/* I/O stats */
		u32 input;
		u32 output;
	} stats;
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
 * Checks whether the process with given id exists
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
 * Returns the file-number for the given file-descriptor
 *
 * @param fd the file-descriptor
 * @return the file-number or < 0 if the fd is invalid
 */
tFileNo proc_fdToFile(tFD fd);

/**
 * Searches for a free file-descriptor
 *
 * @return the file-descriptor or the error-code (< 0)
 */
tFD proc_getFreeFd(void);

/**
 * Associates the given file-descriptor with the given file-number
 *
 * @param fd the file-descriptor
 * @param fileNo the file-number
 * @return 0 on success
 */
s32 proc_assocFd(tFD fd,tFileNo fileNo);

/**
 * Duplicates the given file-descriptor
 *
 * @param fd the file-descriptor
 * @return the error-code or the new file-descriptor
 */
tFD proc_dupFd(tFD fd);

/**
 * Redirects <src> to <dst>. <src> will be closed. Note that both fds have to exist!
 *
 * @param src the source-file-descriptor
 * @param dst the destination-file-descriptor
 * @return the error-code or 0 if successfull
 */
s32 proc_redirFd(tFD src,tFD dst);

/**
 * Releases the given file-descriptor (marks it unused)
 *
 * @param fd the file-descriptor
 * @return the file-number that was associated with the fd (or ERR_INVALID_FD)
 */
tFileNo proc_unassocFd(tFD fd);

/**
 * Searches for a process with given binary
 *
 * @param bin the binary
 * @param rno will be set to the region-number if found
 * @return the process with the binary or NULL if not found
 */
sProc *proc_getProcWithBin(sBinDesc *bin,tVMRegNo *rno);

/**
 * Determines the least recently used region of all processes
 *
 * @return the region (may be NULL)
 */
sRegion *proc_getLRURegion(void);

/**
 * Determines the mem-usage of all processes
 *
 * @param paging will point to the number of bytes used for paging-structures
 * @param dataShared will point to the number of shared bytes
 * @param dataOwn will point to the number of own bytes
 * @param dataReal will point to the number of really used bytes (considers sharing)
 */
void proc_getMemUsage(u32 *paging,u32 *dataShared,u32 *dataOwn,u32 *dataReal);

/**
 * Determines whether the given process has a child
 *
 * @param pid the process-id
 * @return true if it has a child
 */
bool proc_hasChild(tPid pid);

/**
 * Clones the current process into the given one, gives the new process a clone of the current
 * thread and saves this thread in proc_clone() so that it will start there on thread_resume().
 * The function returns -1 if there is not enough memory.
 *
 * @param newPid the target-pid
 * @param isVM86 true if it should be a VM86-task
 * @return -1 if an error occurred, 0 for parent, 1 for child
 */
s32 proc_clone(tPid newPid,bool isVM86);

/**
 * Starts a new thread at given entry-point. Will clone the kernel-stack from the current thread
 *
 * @param entryPoint the address where to start
 * @param arg the argument
 * @return < 0 if an error occurred or the new tid
 */
s32 proc_startThread(u32 entryPoint,void *arg);

/**
 * Destroys the current thread. If it's the only thread in the process, the complete process will
 * destroyed.
 * Note that the actual deletion will be done later!
 *
 * @param exitCode the exit-code
 */
void proc_destroyThread(s32 exitCode);

/**
 * Removes all regions from the given process
 *
 * @param p the process
 * @param remStack wether the stack should be removed too
 */
void proc_removeRegions(sProc *p,bool remStack);

/**
 * Stores the exit-state of the first terminated child-process of <ppid> into <state>
 *
 * @param ppid the parent-pid
 * @param state the pointer to the state (may be NULL!)
 * @return the pid on success
 */
s32 proc_getExitState(tPid ppid,sExitState *state);

/**
 * Marks the given process as zombie and notifies the waiting parent thread. As soon as the parent
 * thread fetches the exit-state we'll kill the process
 *
 * @param p the process
 * @param exitCode the exit-code to store
 * @param signal the signal with which it was killed (SIG_COUNT if none)
 */
void proc_terminate(sProc *p,s32 exitCode,tSig signal);

/**
 * Kills the given process, that means all structures will be destroyed and the memory free'd.
 * This is not possible with the current process!
 *
 * @param p the process
 */
void proc_kill(sProc *p);

/**
 * Copies the arguments (for exec) in <args> to <*argBuffer> and takes care that everything
 * is ok. <*argBuffer> will be allocated on the heap, so that is has to be free'd when you're done.
 *
 * @param args the arguments to copy
 * @param argBuffer will point to the location where it has been copied (to be used by
 * 	proc_setupUserStack())
 * @param size will point to the size the arguments take in <argBuffer>
 * @param fromUser whether the arguments are from user-space
 * @return the number of arguments on success or < 0
 */
s32 proc_buildArgs(const char *const *args,char **argBuffer,u32 *size,bool fromUser);

/**
 * Setups the user-stack for given interrupt-stack of the current process
 *
 * @param frame the interrupt-stack-frame
 * @param argc the argument-count
 * @param args the arguments on after another, allocated on the heap; may be NULL
 * @param argsSize the total number of bytes for the arguments (just the data)
 * @param info startup-info
 * @return true if successfull
 */
bool proc_setupUserStack(sIntrptStackFrame *frame,u32 argc,char *args,u32 argsSize,
		sStartupInfo *info);

/**
 * Setups the start of execution in user-mode for given interrupt-stack
 *
 * @param frame the interrupt-stack-frame
 * @param entryPoint the entry-point for the thread
 */
void proc_setupStart(sIntrptStackFrame *frame,u32 entryPoint);

#if DEBUGGING

/**
 * Starts profiling all processes
 */
void proc_dbg_startProf(void);

/**
 * Stops profiling all processes and prints the result
 */
void proc_dbg_stopProf(void);

/**
 * Prints all existing processes
 */
void proc_dbg_printAll(void);

/**
 * Prints the regions of all existing processes
 */
void proc_dbg_printAllRegions(void);

/**
 * Prints the given parts of the page-directory for all existing processes
 *
 * @param parts the parts (see paging_dbg_printPageDirOf)
 * @param regions wether to print the regions too
 */
void proc_dbg_printAllPDs(u8 parts,bool regions);

/**
 * Prints the given process
 *
 * @param p the pointer to the process
 */
void proc_dbg_print(sProc *p);

#endif

#endif /*PROC_H_*/
