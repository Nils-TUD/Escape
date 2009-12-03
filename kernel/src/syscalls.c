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

#include <common.h>
#include <syscalls.h>
#include <machine/intrpt.h>
#include <machine/timer.h>
#include <machine/gdt.h>
#include <task/proc.h>
#include <task/signals.h>
#include <task/ioports.h>
#include <task/lock.h>
#include <task/sched.h>
#include <task/elf.h>
#include <mem/paging.h>
#include <mem/pmem.h>
#include <mem/text.h>
#include <mem/sharedmem.h>
#include <mem/kheap.h>
#include <vfs/vfs.h>
#include <vfs/node.h>
#include <vfs/real.h>
#include <vfs/driver.h>
#include <vfs/rw.h>
#include <util.h>
#include <debug.h>
#include <video.h>
#include <multiboot.h>
#include <string.h>
#include <assert.h>
#include <errors.h>
#include <messages.h>

/* service-types, as defined in libc/include/esc/service.h */
#define SERV_DEFAULT				1
#define SERV_FS						2
#define SERV_DRIVER					4
#define SERV_ALL					(SERV_DEFAULT | SERV_FS | SERV_DRIVER)

/* some convenience-macros */
#define SYSC_SETERROR(stack,errorCode)	((stack)->ebx = (errorCode))
#define SYSC_ERROR(stack,errorCode)		{ ((stack)->ebx = (errorCode)); return; }
#define SYSC_RET1(stack,val)			((stack)->eax = (val))
#define SYSC_RET2(stack,val)			((stack)->edx = (val))
#define SYSC_NUMBER(stack)				((stack)->eax)
#define SYSC_ARG1(stack)				((stack)->ecx)
#define SYSC_ARG2(stack)				((stack)->edx)
#define SYSC_ARG3(stack)				(*((u32*)(stack)->uesp))
#define SYSC_ARG4(stack)				(*(((u32*)(stack)->uesp) + 1))

/* syscall-handlers */
typedef void (*fSyscall)(sIntrptStackFrame *stack);

/* for syscall-definitions */
typedef struct {
	fSyscall handler;
	u8 argCount;
} sSyscall;

/**
 * Loads the multiboot-modules. This is intended for initloader only!
 */
static void sysc_loadMods(sIntrptStackFrame *stack);
/**
 * Returns the pid of the current process
 *
 * @return tPid the pid
 */
static void sysc_getpid(sIntrptStackFrame *stack);
/**
 * Returns the tid of the current thread
 *
 * @return tTid the thread-id
 */
static void sysc_gettid(sIntrptStackFrame *stack);
/**
 * @return u32 the number of threads of the current process
 */
static void sysc_getThreadCount(sIntrptStackFrame *stack);
/**
 * Returns the parent-pid of the given process
 *
 * @param pid the process-id
 * @return tPid the parent-pid
 */
static void sysc_getppid(sIntrptStackFrame *stack);
/**
 * Temporary syscall to print out a character
 */
static void sysc_debugc(sIntrptStackFrame *stack);
/**
 * Clones the current process
 *
 * @return tPid 0 for the child, the child-pid for the parent-process
 */
static void sysc_fork(sIntrptStackFrame *stack);
/**
 * Starts a new thread
 *
 * @param entryPoint the entry-point
 * @param char** arguments
 * @return tTid 0 for the new thread, the new thread-id for the current thread
 */
static void sysc_startThread(sIntrptStackFrame *stack);
/**
 * Destroys the process and issues a context-switch
 *
 * @param u32 the exit-code
 */
static void sysc_exit(sIntrptStackFrame *stack);
/**
 * Open-syscall. Opens a given path with given mode and returns the file-descriptor to use
 * to the user.
 *
 * @param const char* the vfs-filename
 * @param u32 flags (read / write)
 * @return tFD the file-descriptor
 */
static void sysc_open(sIntrptStackFrame *stack);
/**
 * Determines the current file-position
 *
 * @param tFD file-descriptor
 * @param u32* file-position
 * @return s32 0 on success
 */
static void sysc_tell(sIntrptStackFrame *stack);
/**
 * Tests wether we are at the end of the file (or if there is a message for service-usages)
 *
 * @param tFD file-descriptor
 * @return bool true if we are at EOF
 */
static void sysc_eof(sIntrptStackFrame *stack);
/**
 * Changes the position in the given file
 *
 * @param tFD the file-descriptor
 * @param s32 the offset
 * @param u32 the seek-type
 * @return the new position on success
 */
static void sysc_seek(sIntrptStackFrame *stack);
/**
 * Read-syscall. Reads a given number of bytes in a given file at the current position
 *
 * @param tFD file-descriptor
 * @param void* buffer
 * @param u32 number of bytes
 * @return u32 the number of read bytes
 */
static void sysc_read(sIntrptStackFrame *stack);
/**
 * Write-syscall. Writes a given number of bytes to a given file at the current position
 *
 * @param tFD file-descriptor
 * @param void* buffer
 * @param u32 number of bytes
 * @return u32 the number of written bytes
 */
static void sysc_write(sIntrptStackFrame *stack);
/**
 * Duplicates the given file-descriptor
 *
 * @param tFD the file-descriptor
 * @return tFD the error-code or the new file-descriptor
 */
static void sysc_dupFd(sIntrptStackFrame *stack);
/**
 * Redirects <src> to <dst>. <src> will be closed. Note that both fds have to exist!
 *
 * @param tFD src the source-file-descriptor
 * @param tFD dst the destination-file-descriptor
 * @return s32 the error-code or 0 if successfull
 */
static void sysc_redirFd(sIntrptStackFrame *stack);
/**
 * Closes the given file-descriptor
 *
 * @param tFD the file-descriptor
 */
static void sysc_close(sIntrptStackFrame *stack);
/**
 * Registers a service
 *
 * @param const char* name of the service
 * @param u8 type: SINGLE-PIPE OR MULTI-PIPE
 * @return tServ the service-id if successfull
 */
static void sysc_regService(sIntrptStackFrame *stack);
/**
 * Unregisters a service
 *
 * @param tServ the service-id
 * @return s32 0 on success or a negative error-code
 */
static void sysc_unregService(sIntrptStackFrame *stack);
/**
 * For services: Looks wether a client wants to be served and returns a file-descriptor
 * for it.
 *
 * @param tServ* services an array with service-ids to check
 * @param u32 count the size of <services>
 * @param tServ* serv will be set to the service from which the client has been taken
 * @return tFD the file-descriptor to use
 */
static void sysc_getClient(sIntrptStackFrame *stack);
/**
 * For services: Returns the file-descriptor for a specific client
 *
 * @param tServ the service-id
 * @param tTid the thread-id
 * @return tFD the file-descriptor
 */
static void sysc_getClientThread(sIntrptStackFrame *stack);
/**
 * Changes the process-size
 *
 * @param u32 number of pages
 * @return u32 the previous number of text+data pages
 */
static void sysc_changeSize(sIntrptStackFrame *stack);
/**
 * Maps physical memory in the virtual user-space
 *
 * @param u32 physical address
 * @param u32 number of bytes
 * @return void* the start-address
 */
static void sysc_mapPhysical(sIntrptStackFrame *stack);
/**
 * Blocks the process until an event occurrs
 */
static void sysc_wait(sIntrptStackFrame *stack);
/**
 * Waits until a child has terminated
 *
 * @param sExitState* will be filled with information about the terminated process
 * @return s32 o on success
 */
static void sysc_waitChild(sIntrptStackFrame *stack);
/**
 * Blocks the process for a given number of milliseconds
 *
 * @param u32 the number of msecs
 * @return s32 0 on success or a negative error-code
 */
static void sysc_sleep(sIntrptStackFrame *stack);
/**
 * Releases the CPU (reschedule)
 */
static void sysc_yield(sIntrptStackFrame *stack);
/**
 * Requests some IO-ports
 *
 * @param u16 start-port
 * @param u16 number of ports
 * @return s32 0 if successfull or a negative error-code
 */
static void sysc_requestIOPorts(sIntrptStackFrame *stack);
/**
 * Releases some IO-ports
 *
 * @param u16 start-port
 * @param u16 number of ports
 * @return s32 0 if successfull or a negative error-code
 */
static void sysc_releaseIOPorts(sIntrptStackFrame *stack);
/**
 * Sets a handler-function for a specific signal
 *
 * @param tSig signal
 * @param fSigHandler handler
 * @return s32 0 if no error
 */
static void sysc_setSigHandler(sIntrptStackFrame *stack);
/**
 * Unsets the handler-function for a specific signal
 *
 * @param tSig signal
 * @return s32 0 if no error
 */
static void sysc_unsetSigHandler(sIntrptStackFrame *stack);
/**
 * Acknoledges that the processing of a signal is finished
 *
 * @return s32 0 if no error
 */
static void sysc_ackSignal(sIntrptStackFrame *stack);
/**
 * Sends a signal to a process
 *
 * @param tPid pid
 * @param tSig signal
 * @return s32 0 if no error
 */
static void sysc_sendSignalTo(sIntrptStackFrame *stack);
/**
 * Exchanges the process-data with another program
 *
 * @param char* the program-path
 */
static void sysc_exec(sIntrptStackFrame *stack);
/**
 * Retrieves information about the given file
 *
 * @param const char* path the path of the file
 * @param tFileInfo* info will be filled
 * @return s32 0 on success
 */
static void sysc_stat(sIntrptStackFrame *stack);
/**
 * Just intended for debugging. May be used for anything :)
 * It's just a system-call thats used by nothing else, so we can use it e.g. for printing
 * debugging infos in the kernel to points of time controlled by user-apps.
 */
static void sysc_debug(sIntrptStackFrame *stack);
/**
 * Creates a shared-memory region
 *
 * @param char* the name
 * @param u32 number of bytes
 * @return s32 the address on success, negative error-code otherwise
 */
static void sysc_createSharedMem(sIntrptStackFrame *stack);
/**
 * Joines a shared-memory region
 *
 * @param char* the name
 * @return s32 the address on success, negative error-code otherwise
 */
static void sysc_joinSharedMem(sIntrptStackFrame *stack);
/**
 * Leaves a shared-memory region
 *
 * @param char* the name
 * @return s32 the address on success, negative error-code otherwise
 */
static void sysc_leaveSharedMem(sIntrptStackFrame *stack);
/**
 * Destroys a shared-memory region
 *
 * @param char* the name
 * @return s32 the address on success, negative error-code otherwise
 */
static void sysc_destroySharedMem(sIntrptStackFrame *stack);
/**
 * Aquire a lock; wait until its unlocked, if necessary
 *
 * @param ident the lock-ident
 * @return s32 0 on success
 */
static void sysc_lock(sIntrptStackFrame *stack);
/**
 * Releases a lock
 *
 * @param ident the lock-ident
 * @return s32 0 on success
 */
static void sysc_unlock(sIntrptStackFrame *stack);
/**
 * Sends a message
 *
 * @param tFD the file-descriptor
 * @param tMsgId the msg-id
 * @param u8 * the data
 * @param u32 the size of the data
 * @return s32 0 on success
 */
static void sysc_send(sIntrptStackFrame *stack);
/**
 * Receives a message
 *
 * @param tFD the file-descriptor
 * @param tMsgId the msg-id
 * @param u8 * the data
 * @return s32 the message-size on success
 */
static void sysc_receive(sIntrptStackFrame *stack);
/**
 * Performs IO-Control on the device that is identified by the given file-descriptor
 *
 * @param tFD the file-descriptor
 * @param u32 the command
 * @param u8 * the data (may be NULL)
 * @param u32 the size of the data
 * @return s32 0 on success
 */
static void sysc_ioctl(sIntrptStackFrame *stack);
/**
 * For drivers: Sets wether read() has currently something to read or not
 *
 * @param tServ the service-id
 * @param bool wether there is data to read
 * @return s32 0 on success
 */
static void sysc_setDataReadable(sIntrptStackFrame *stack);
/**
 * Returns the cpu-cycles for the current thread
 *
 * @return u64 the cpu-cycles
 */
static void sysc_getCycles(sIntrptStackFrame *stack);
/**
 * Writes all dirty objects of the filesystem to disk
 *
 * @return s32 0 on success
 */
static void sysc_sync(sIntrptStackFrame *stack);
/**
 * Creates a hardlink at <newPath> which points to <oldPath>
 *
 * @param char* the old path
 * @param char* the new path
 * @return s32 0 on success
 */
static void sysc_link(sIntrptStackFrame *stack);
/**
 * Unlinks the given path. That means, the directory-entry will be removed and if there are no
 * more references to the inode, it will be removed.
 *
 * @param char* path
 * @return 0 on success
 */
static void sysc_unlink(sIntrptStackFrame *stack);
/**
 * Creates the given directory. Expects that all except the last path-component exist.
 *
 * @param char* the path
 * @return s32 0 on success
 */
static void sysc_mkdir(sIntrptStackFrame *stack);
/**
 * Removes the given directory. Expects that the directory is empty (except '.' and '..')
 *
 * @param char* the path
 * @return s32 0 on success
 */
static void sysc_rmdir(sIntrptStackFrame *stack);
/**
 * Mounts <device> at <path> with fs <type>
 *
 * @param char* the device-path
 * @param char* the path to mount at
 * @param u16 the fs-type
 * @return s32 0 on success
 */
static void sysc_mount(sIntrptStackFrame *stack);

/**
 * Unmounts the device mounted at <path>
 *
 * @param char* the path
 * @return s32 0 on success
 */
static void sysc_unmount(sIntrptStackFrame *stack);

/* our syscalls */
static sSyscall syscalls[] = {
	/* 0 */		{sysc_getpid,				0},
	/* 1 */		{sysc_getppid,				1},
	/* 2 */ 	{sysc_debugc,				1},
	/* 3 */		{sysc_fork,					0},
	/* 4 */ 	{sysc_exit,					1},
	/* 5 */ 	{sysc_open,					2},
	/* 6 */ 	{sysc_close,				1},
	/* 7 */ 	{sysc_read,					3},
	/* 8 */		{sysc_regService,			2},
	/* 9 */ 	{sysc_unregService,			1},
	/* 10 */	{sysc_changeSize,			1},
	/* 11 */	{sysc_mapPhysical,			2},
	/* 12 */	{sysc_write,				3},
	/* 13 */	{sysc_yield,				0},
	/* 14 */	{sysc_getClient,			3},
	/* 15 */	{sysc_requestIOPorts,		2},
	/* 16 */	{sysc_releaseIOPorts,		2},
	/* 17 */	{sysc_dupFd,				1},
	/* 18 */	{sysc_redirFd,				2},
	/* 19 */	{sysc_wait,					1},
	/* 20 */	{sysc_setSigHandler,		2},
	/* 21 */	{sysc_unsetSigHandler,		1},
	/* 22 */	{sysc_ackSignal,			0},
	/* 23 */	{sysc_sendSignalTo,			3},
	/* 24 */	{sysc_exec,					2},
	/* 25 */	{sysc_eof,					1},
	/* 26 */	{sysc_loadMods,				0},
	/* 27 */	{sysc_sleep,				1},
	/* 28 */	{sysc_seek,					2},
	/* 29 */	{sysc_stat,					2},
	/* 30 */	{sysc_debug,				0},
	/* 31 */	{sysc_createSharedMem,		2},
	/* 32 */	{sysc_joinSharedMem,		1},
	/* 33 */	{sysc_leaveSharedMem,		1},
	/* 34 */	{sysc_destroySharedMem,		1},
	/* 35 */	{sysc_getClientThread,		2},
	/* 36 */	{sysc_lock,					1},
	/* 37 */	{sysc_unlock,				1},
	/* 38 */	{sysc_startThread,			2},
	/* 39 */	{sysc_gettid,				0},
	/* 40 */	{sysc_getThreadCount,		0},
	/* 41 */	{sysc_send,					4},
	/* 42 */	{sysc_receive,				3},
	/* 43 */	{sysc_ioctl,				4},
	/* 44 */	{sysc_setDataReadable,		2},
	/* 45 */	{sysc_getCycles,			0},
	/* 46 */	{sysc_sync,					0},
	/* 47 */	{sysc_link,					2},
	/* 48 */	{sysc_unlink,				1},
	/* 49 */	{sysc_mkdir,				1},
	/* 50 */	{sysc_rmdir,				1},
	/* 51 */	{sysc_mount,				3},
	/* 52 */	{sysc_unmount,				1},
	/* 53 */	{sysc_waitChild,			1},
	/* 54 */	{sysc_tell,					2},
};

void sysc_handle(sIntrptStackFrame *stack) {
	u32 sysCallNo = SYSC_NUMBER(stack);
	if(sysCallNo < ARRAY_SIZE(syscalls)) {
		u32 argCount = syscalls[sysCallNo].argCount;
		u32 ebxSave = stack->ebx;
		/* handle copy-on-write (the first 2 args are passed in registers) */
		/* TODO maybe we need more stack-pages */
		if(argCount > 2)
			paging_isRangeUserWritable((u32)stack->uesp,sizeof(u32) * (argCount - 2));

		/* no error by default */
		SYSC_SETERROR(stack,0);
		syscalls[sysCallNo].handler(stack);

		/* set error-code */
		stack->ecx = stack->ebx;
		stack->ebx = ebxSave;
	}
}

static void sysc_loadMods(sIntrptStackFrame *stack) {
	UNUSED(stack);
	mboot_loadModules(intrpt_getCurStack());
}

static void sysc_getpid(sIntrptStackFrame *stack) {
	sProc *p = proc_getRunning();
	SYSC_RET1(stack,p->pid);
}

static void sysc_gettid(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	SYSC_RET1(stack,t->tid);
}

static void sysc_getThreadCount(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	SYSC_RET1(stack,sll_length(t->proc->threads));
}

static void sysc_getppid(sIntrptStackFrame *stack) {
	tPid pid = (tPid)SYSC_ARG1(stack);
	sProc *p;

	if(!proc_exists(pid))
		SYSC_ERROR(stack,ERR_INVALID_PID);

	p = proc_getByPid(pid);
	SYSC_RET1(stack,p->parentPid);
}

static void sysc_debugc(sIntrptStackFrame *stack) {
	vid_putchar((char)SYSC_ARG1(stack));
}

static void sysc_fork(sIntrptStackFrame *stack) {
	tPid newPid = proc_getFreePid();
	s32 res;

	/* no free slot? */
	if(newPid == INVALID_PID)
		SYSC_ERROR(stack,ERR_NO_FREE_PROCS);

	res = proc_clone(newPid);

	/* error? */
	if(res < 0)
		SYSC_ERROR(stack,res);
	/* child? */
	if(res == 1)
		SYSC_RET1(stack,0);
	/* parent */
	else
		SYSC_RET1(stack,newPid);
}

static void sysc_startThread(sIntrptStackFrame *stack) {
	u32 entryPoint = SYSC_ARG1(stack);
	char **args = (char**)SYSC_ARG2(stack);
	char *argBuffer = NULL;
	u32 argSize = 0;
	s32 res,argc = 0;

	/* build arguments */
	if(args != NULL) {
		argc = proc_buildArgs(args,&argBuffer,&argSize,true);
		if(argc < 0)
			SYSC_ERROR(stack,argc);
	}

	res = proc_startThread(entryPoint,argc,argBuffer,argSize);
	if(res < 0)
		SYSC_ERROR(stack,res);
	/* free arguments when new thread returns */
	if(res == 0)
		kheap_free(argBuffer);
	SYSC_RET1(stack,res);
}

static void sysc_exit(sIntrptStackFrame *stack) {
	s32 exitCode = (s32)SYSC_ARG1(stack);
	proc_destroyThread(exitCode);
	thread_switch();
}

static void sysc_open(sIntrptStackFrame *stack) {
	char *path = (char*)SYSC_ARG1(stack);
	s32 pathLen;
	u16 flags;
	tInodeNo nodeNo = 0;
	bool created,isVirt = false;
	tFileNo file;
	tFD fd;
	s32 err;
	sThread *t = thread_getRunning();

	/* at first make sure that we'll cause no page-fault */
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	pathLen = strnlen(path,MAX_PATH_LEN);
	if(pathLen == -1)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* check flags */
	flags = ((u16)SYSC_ARG2(stack)) & (VFS_WRITE | VFS_READ | VFS_CREATE | VFS_TRUNCATE | VFS_APPEND);
	if((flags & (VFS_READ | VFS_WRITE)) == 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* resolve path */
	err = vfsn_resolvePath(path,&nodeNo,&created,flags | VFS_CONNECT);
	if(err == ERR_REAL_PATH) {
		/* send msg to fs and wait for reply */
		file = vfsr_openFile(t->tid,flags,path);
		if(file < 0)
			SYSC_ERROR(stack,file);

		/* get free fd */
		fd = thread_getFreeFd();
		if(fd < 0) {
			vfs_closeFile(t->tid,file);
			SYSC_ERROR(stack,fd);
		}
	}
	else {
		/* handle virtual files */
		if(err < 0)
			SYSC_ERROR(stack,err);

		/* get free fd */
		fd = thread_getFreeFd();
		if(fd < 0)
			SYSC_ERROR(stack,fd);
		/* store wether we have created the node */
		if(created)
			flags |= VFS_CREATED;
		/* open file */
		file = vfs_openFile(t->tid,flags,nodeNo,VFS_DEV_NO);
		if(file < 0)
			SYSC_ERROR(stack,file);
		isVirt = true;
	}

	/* assoc fd with file */
	err = thread_assocFd(fd,file);
	if(err < 0)
		SYSC_ERROR(stack,err);

	/* if it is a driver, call the driver open-command */
	if(isVirt) {
		sVFSNode *node = vfsn_getNode(nodeNo);
		if((node->mode & MODE_TYPE_SERVUSE) && IS_DRIVER(node->parent->mode)) {
			err = vfsdrv_open(t->tid,file,node,flags);
			/* if this went wrong, undo everything and report an error to the user */
			if(err < 0) {
				thread_unassocFd(fd);
				vfs_closeFile(t->tid,file);
				SYSC_ERROR(stack,err);
			}
		}
	}

	/* append? */
	if(flags & VFS_APPEND) {
		err = vfs_seek(t->tid,file,0,SEEK_END);
		if(err < 0)
			SYSC_ERROR(stack,err);
	}

	/* return fd */
	SYSC_RET1(stack,fd);
}

static void sysc_tell(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	u32 *pos = (u32*)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	tFileNo file;

	if(!paging_isRangeUserWritable((u32)pos,sizeof(u32)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	*pos = vfs_tell(t->tid,file);
	SYSC_RET1(stack,0);
}

static void sysc_eof(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();
	tFileNo file;
	bool eof;

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	eof = vfs_eof(t->tid,file);
	SYSC_RET1(stack,eof);
}

static void sysc_seek(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	s32 offset = (s32)SYSC_ARG2(stack);
	u32 whence = SYSC_ARG3(stack);
	sThread *t = thread_getRunning();
	tFileNo file;
	s32 res;

	if(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	res = vfs_seek(t->tid,file,offset,whence);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_read(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	u8 *buffer = (u8*)SYSC_ARG2(stack);
	u32 count = SYSC_ARG3(stack);
	sThread *t = thread_getRunning();
	s32 readBytes;
	tFileNo file;

	/* validate count and buffer */
	if(count <= 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isRangeUserWritable((u32)buffer,count))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* read */
	readBytes = vfs_readFile(t->tid,file,buffer,count);
	if(readBytes < 0)
		SYSC_ERROR(stack,readBytes);

	SYSC_RET1(stack,readBytes);
}

static void sysc_write(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	u8 *buffer = (u8*)SYSC_ARG2(stack);
	u32 count = SYSC_ARG3(stack);
	sThread *t = thread_getRunning();
	s32 writtenBytes;
	tFileNo file;

	/* validate count and buffer */
	if(count <= 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isRangeUserReadable((u32)buffer,count))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* read */
	writtenBytes = vfs_writeFile(t->tid,file,buffer,count);
	if(writtenBytes < 0)
		SYSC_ERROR(stack,writtenBytes);

	SYSC_RET1(stack,writtenBytes);
}

static void sysc_ioctl(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	u32 cmd = SYSC_ARG2(stack);
	u8 *data = (u8*)SYSC_ARG3(stack);
	u32 size = SYSC_ARG4(stack);
	sThread *t = thread_getRunning();
	tFileNo file;
	s32 res;

	if(data != NULL && size == 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	/* TODO always writable ok? */
	if(data != NULL && !paging_isRangeUserWritable((u32)data,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* perform io-control */
	res = vfs_ioctl(t->tid,file,cmd,data,size);
	if(res < 0)
		SYSC_ERROR(stack,res);

	SYSC_RET1(stack,res);
}

static void sysc_send(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	tMsgId id = (tMsgId)SYSC_ARG2(stack);
	u8 *data = (u8*)SYSC_ARG3(stack);
	u32 size = SYSC_ARG4(stack);
	sThread *t = thread_getRunning();
	tFileNo file;
	s32 res;

	/* validate size and data */
	if(size <= 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isRangeUserReadable((u32)data,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* send msg */
	res = vfs_sendMsg(t->tid,file,id,data,size);
	if(res < 0)
		SYSC_ERROR(stack,res);

	SYSC_RET1(stack,res);
}

static void sysc_receive(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	tMsgId *id = (tMsgId*)SYSC_ARG2(stack);
	u8 *data = (u8*)SYSC_ARG3(stack);
	u32 size = SYSC_ARG4(stack);
	sThread *t = thread_getRunning();
	tFileNo file;
	s32 res;

	/* validate id and data */
	if(!paging_isRangeUserWritable((u32)id,sizeof(tMsgId)) ||
			!paging_isRangeUserWritable((u32)data,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* send msg */
	res = vfs_receiveMsg(t->tid,file,id,data,size);
	if(res < 0)
		SYSC_ERROR(stack,res);

	SYSC_RET1(stack,res);
}

static void sysc_dupFd(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	tFD res;

	res = thread_dupFd(fd);
	if(res < 0)
		SYSC_ERROR(stack,res);

	SYSC_RET1(stack,res);
}

static void sysc_redirFd(sIntrptStackFrame *stack) {
	tFD src = (tFD)SYSC_ARG1(stack);
	tFD dst = (tFD)SYSC_ARG2(stack);
	s32 err;

	err = thread_redirFd(src,dst);
	if(err < 0)
		SYSC_ERROR(stack,err);

	SYSC_RET1(stack,err);
}

static void sysc_close(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();

	/* unassoc fd */
	tFileNo fileNo = thread_unassocFd(fd);
	if(fileNo < 0)
		return;

	/* close file */
	vfs_closeFile(t->tid,fileNo);
}

static void sysc_regService(sIntrptStackFrame *stack) {
	const char *name = (const char*)SYSC_ARG1(stack);
	u32 type = (u32)SYSC_ARG2(stack);
	u32 vtype;
	sThread *t = thread_getRunning();
	tServ res;

	/* check type */
	if((type & SERV_ALL) == 0 || (type & ~SERV_ALL) != 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* convert and check type */
	vtype = 0;
	if(type & SERV_FS)
		vtype |= MODE_SERVICE_FS;
	else if(type & SERV_DRIVER)
		vtype |= MODE_SERVICE_DRIVER;

	res = vfs_createService(t->tid,name,vtype);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_unregService(sIntrptStackFrame *stack) {
	tServ id = SYSC_ARG1(stack);
	sThread *t = thread_getRunning();
	s32 err;

	/* check node-number */
	if(!vfsn_isValidNodeNo(id))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* remove the service */
	err = vfs_removeService(t->tid,id);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,0);
}

static void sysc_setDataReadable(sIntrptStackFrame *stack) {
	tServ id = SYSC_ARG1(stack);
	bool readable = (bool)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	s32 err;

	/* check node-number */
	if(!vfsn_isValidNodeNo(id))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	err = vfs_setDataReadable(t->tid,id,readable);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,0);
}

static void sysc_getClient(sIntrptStackFrame *stack) {
	tServ *ids = (tServ*)SYSC_ARG1(stack);
	u32 count = SYSC_ARG2(stack);
	tServ *client = (tServ*)SYSC_ARG3(stack);
	sThread *t = thread_getRunning();
	tFD fd;
	s32 res;
	tFileNo file;

	/* check arguments. limit count a little bit to prevent overflow */
	if(count <= 0 || count > 32 || ids == NULL ||
			!paging_isRangeUserReadable((u32)ids,count * sizeof(tServ)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(client == NULL || !paging_isRangeUserWritable((u32)client,sizeof(tServ)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* we need a file-desc */
	fd = thread_getFreeFd();
	if(fd < 0)
		SYSC_ERROR(stack,fd);

	/* open a client */
	file = vfs_openClient(t->tid,(tInodeNo*)ids,count,(tInodeNo*)client);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* associate fd with file */
	res = thread_assocFd(fd,file);
	if(res < 0) {
		/* we have already opened the file */
		vfs_closeFile(t->tid,file);
		SYSC_ERROR(stack,res);
	}

	SYSC_RET1(stack,fd);
}

static void sysc_getClientThread(sIntrptStackFrame *stack) {
	tServ id = (tServ)SYSC_ARG1(stack);
	tTid tid = (tPid)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	tFD fd;
	tFileNo file;
	s32 res;

	if(thread_getById(tid) == NULL)
		SYSC_ERROR(stack,ERR_INVALID_TID);

	/* we need a file-desc */
	fd = thread_getFreeFd();
	if(fd < 0)
		SYSC_ERROR(stack,fd);

	/* open client */
	file = vfs_openClientThread(t->tid,id,tid);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* associate fd with file */
	res = thread_assocFd(fd,file);
	if(res < 0) {
		/* we have already opened the file */
		vfs_closeFile(t->tid,file);
		SYSC_ERROR(stack,res);
	}

	SYSC_RET1(stack,fd);
}

static void sysc_changeSize(sIntrptStackFrame *stack) {
	s32 count = SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	u32 oldEnd = p->textPages + p->dataPages;

	/* check argument */
	if(count > 0) {
		if(!proc_segSizesValid(p->textPages,p->dataPages + count,p->stackPages))
			SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
	}
	else if(count == 0) {
		SYSC_RET1(stack,oldEnd);
		return;
	}
	else if((u32)-count > p->dataPages)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* change size */
	if(proc_changeSize(count,CHG_DATA) == false)
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);

	SYSC_RET1(stack,oldEnd);
}

static void sysc_mapPhysical(sIntrptStackFrame *stack) {
	u32 addr,origPages;
	u32 phys = SYSC_ARG1(stack);
	u32 pages = BYTES_2_PAGES(SYSC_ARG2(stack));
	sProc *p = proc_getRunning();
	u32 i,*frames;

	/* trying to map memory in kernel area? */
	/* TODO is this ok? */
	if(OVERLAPS(phys,phys + pages,KERNEL_P_ADDR,KERNEL_P_ADDR + PAGE_SIZE * PT_ENTRY_COUNT))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* determine start-address */
	addr = (p->textPages + p->dataPages) * PAGE_SIZE;
	origPages = p->textPages + p->dataPages;

	/* not enough mem? */
	if(mm_getFreeFrmCount(MM_DEF) < paging_countFramesForMap(addr,pages))
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);

	/* invalid segment sizes? */
	if(!proc_segSizesValid(p->textPages,p->dataPages + pages,p->stackPages))
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);

	/* we have to allocate temporary space for the frame-address :( */
	frames = (u32*)kheap_alloc(sizeof(u32) * pages);
	if(frames == NULL)
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);

	for(i = 0; i < pages; i++) {
		frames[i] = phys / PAGE_SIZE;
		phys += PAGE_SIZE;
	}
	/* map the physical memory and free the temporary memory */
	paging_map(addr,frames,pages,PG_WRITABLE | PG_NOFREE,false);
	kheap_free(frames);

	/* increase datapages */
	p->dataPages += pages;
	/* return start-addr */
	SYSC_RET1(stack,addr);
}

static void sysc_wait(sIntrptStackFrame *stack) {
	u8 events = (u8)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();

	if((events & ~(EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) != 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* check wether there is a chance that we'll wake up again */
	if(!vfs_msgAvailableFor(t->tid,events)) {
		thread_wait(t->tid,events);
		thread_switch();
	}
}

static void sysc_waitChild(sIntrptStackFrame *stack) {
	sExitState *state = (sExitState*)SYSC_ARG1(stack);
	sSLNode *n;
	s32 res;
	sProc *p = proc_getRunning();
	sThread *t = thread_getRunning();

	if(state != NULL && !paging_isRangeUserWritable((u32)state,sizeof(sExitState)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!proc_hasChild(t->proc->pid))
		SYSC_ERROR(stack,ERR_NO_CHILD);

	/* check if there is another thread waiting */
	for(n = sll_begin(p->threads); n != NULL; n = n->next) {
		if(((sThread*)n->data)->events & EV_CHILD_DIED)
			SYSC_ERROR(stack,ERR_THREAD_WAITING);
	}

	/* check if there is already a dead child-proc */
	res = proc_getExitState(p->pid,state);
	if(res < 0) {
		/* wait for child */
		thread_wait(t->tid,EV_CHILD_DIED);
		thread_switch();

		/* we're back again :) */
		/* don't continue here if we were interrupted by a signal */
		if(sig_hasSignalFor(t->tid)) {
			/* stop waiting for event */
			thread_wakeup(t->tid,EV_CHILD_DIED);
			SYSC_ERROR(stack,ERR_INTERRUPTED);
		}
		res = proc_getExitState(p->pid,state);
		if(res < 0)
			SYSC_ERROR(stack,0);
	}

	/* finally kill the process */
	proc_kill(proc_getByPid(res));
	SYSC_RET1(stack,0);
}

static void sysc_sleep(sIntrptStackFrame *stack) {
	u32 msecs = SYSC_ARG1(stack);
	timer_sleepFor(thread_getRunning()->tid,msecs);
	thread_switch();
}

static void sysc_yield(sIntrptStackFrame *stack) {
	UNUSED(stack);

	thread_switch();
}

static void sysc_requestIOPorts(sIntrptStackFrame *stack) {
	u16 start = SYSC_ARG1(stack);
	u16 count = SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	s32 err;

	/* check range */
	if(count == 0 || (u32)start + (u32)count > 0xFFFF)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	err = ioports_request(p,start,count);
	if(err < 0)
		SYSC_ERROR(stack,err);

	tss_setIOMap(p->ioMap);
	SYSC_RET1(stack,0);
}

static void sysc_releaseIOPorts(sIntrptStackFrame *stack) {
	u16 start = SYSC_ARG1(stack);
	u16 count = SYSC_ARG2(stack);
	sProc *p;
	s32 err;

	/* check range */
	if(count == 0 || (u32)start + (u32)count > 0xFFFF)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	p = proc_getRunning();
	err = ioports_release(p,start,count);
	if(err < 0)
		SYSC_ERROR(stack,err);

	tss_setIOMap(p->ioMap);
	SYSC_RET1(stack,0);
}

static void sysc_setSigHandler(sIntrptStackFrame *stack) {
	tSig signal = (tSig)SYSC_ARG1(stack);
	fSigHandler handler = (fSigHandler)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	s32 err;

	/* address should be valid */
	if(!paging_isRangeUserReadable((u32)handler,1))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* check signal */
	if(!sig_canHandle(signal))
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);

	err = sig_setHandler(t->tid,signal,handler);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,err);
}

static void sysc_unsetSigHandler(sIntrptStackFrame *stack) {
	tSig signal = (tSig)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();

	if(!sig_canHandle(signal))
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);

	sig_unsetHandler(t->tid,signal);

	SYSC_RET1(stack,0);
}

static void sysc_ackSignal(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	sig_ackHandling(t->tid);
	SYSC_RET1(stack,0);
}

static void sysc_sendSignalTo(sIntrptStackFrame *stack) {
	tPid pid = (tPid)SYSC_ARG1(stack);
	tSig signal = (tSig)SYSC_ARG2(stack);
	u32 data = SYSC_ARG3(stack);
	sThread *t = thread_getRunning();

	if(!sig_canSend(signal))
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);
	if(pid != INVALID_PID && !proc_exists(pid))
		SYSC_ERROR(stack,ERR_INVALID_PID);

	if(pid != INVALID_PID)
		sig_addSignalFor(pid,signal,data);
	else
		sig_addSignal(signal,data);

	/* choose another thread if we've killed ourself */
	if(t->state != ST_RUNNING)
		thread_switch();

	SYSC_RET1(stack,0);
}

static void sysc_exec(sIntrptStackFrame *stack) {
	char pathSave[MAX_PATH_LEN + 1];
	char *path = (char*)SYSC_ARG1(stack);
	char **args = (char**)SYSC_ARG2(stack);
	char *argBuffer;
	s32 argc,pathLen,res;
	u32 argSize;
	tInodeNo nodeNo;
	sProc *p = proc_getRunning();

	argc = 0;
	argBuffer = NULL;
	if(args != NULL) {
		argc = proc_buildArgs(args,&argBuffer,&argSize,true);
		if(argc < 0)
			SYSC_ERROR(stack,argc);
	}

	/* at first make sure that we'll cause no page-fault */
	if(!sysc_isStringReadable(path)) {
		kheap_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}

	/* save path (we'll overwrite the process-data) */
	pathLen = strlen(path);
	if(pathLen >= MAX_PATH_LEN) {
		kheap_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}
	memcpy(pathSave,path,pathLen + 1);
	path = pathSave;

	/* resolve path; require a path in real fs */
	res = vfsn_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res != ERR_REAL_PATH) {
		kheap_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}

	/* free the current text; free frames if text_free() returns true */
	paging_unmap(0,p->textPages,text_free(p->text,p->pid),false);
	/* ensure that we don't have a text-usage anymore */
	p->text = NULL;
	/* remove process-data */
	proc_changeSize(-p->dataPages,CHG_DATA);
	/* Note that we HAVE TO do it behind the proc_changeSize() call since the data-pages are
	 * still behind the text-pages, no matter if we've already unmapped the text-pages or not,
	 * and proc_changeSize() trusts p->textPages */
	p->textPages = 0;

	/* load program */
	res = elf_loadFromFile(path);
	if(res < 0) {
		/* there is no undo for proc_changeSize() :/ */
		kheap_free(argBuffer);
		proc_terminate(p,res,SIG_COUNT);
		thread_switch();
		return;
	}

	/* copy path so that we can identify the process */
	memcpy(p->command,pathSave + (pathLen > MAX_PROC_NAME_LEN ? (pathLen - MAX_PROC_NAME_LEN) : 0),
			MIN(MAX_PROC_NAME_LEN,pathLen) + 1);

	/* make process ready */
	proc_setupUserStack(stack,argc,argBuffer,argSize);
	proc_setupStart(stack,(u32)res);

	kheap_free(argBuffer);
}

static void sysc_stat(sIntrptStackFrame *stack) {
	char *path = (char*)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	tInodeNo nodeNo;
	u32 len;
	s32 res;

	if(!sysc_isStringReadable(path) || info == NULL ||
			!paging_isRangeUserWritable((u32)info,sizeof(sFileInfo)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	len = strlen(path);
	if(len == 0 || len >= MAX_PATH_LEN)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfsn_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == ERR_REAL_PATH) {
		sThread *t = thread_getRunning();
		res = vfsr_stat(t->tid,path,info);
	}
	else if(res == 0)
		res = vfsn_getNodeInfo(nodeNo,info);

	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

static void sysc_debug(sIntrptStackFrame *stack) {
	UNUSED(stack);
	/*proc_dbg_printAll();
	vfsn_dbg_printTree();*/
	paging_dbg_printOwnPageDir(PD_PART_USER);
}

static void sysc_createSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	u32 byteCount = SYSC_ARG2(stack);
	s32 res;

	if(!sysc_isStringReadable(name) || byteCount == 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_create(name,BYTES_2_PAGES(byteCount));
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res * PAGE_SIZE);
}

static void sysc_joinSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	s32 res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_join(name);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res * PAGE_SIZE);
}

static void sysc_leaveSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	s32 res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_leave(name);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_destroySharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	s32 res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_destroy(name);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_lock(sIntrptStackFrame *stack) {
	u32 ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	s32 res;

	res = lock_aquire(t->tid,global ? INVALID_PID : t->proc->pid,ident);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_unlock(sIntrptStackFrame *stack) {
	u32 ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	s32 res;

	res = lock_release(global ? INVALID_PID : t->proc->pid,ident);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_getCycles(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	uLongLong cycles;
	cycles.val64 = t->kcycleCount.val64 + t->ucycleCount.val64;
	SYSC_RET1(stack,cycles.val32.upper);
	SYSC_RET2(stack,cycles.val32.lower);
}

static void sysc_sync(sIntrptStackFrame *stack) {
	s32 res;
	sThread *t = thread_getRunning();
	res = vfsr_sync(t->tid);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_link(sIntrptStackFrame *stack) {
	s32 res;
	sThread *t = thread_getRunning();
	char *oldPath = (char*)SYSC_ARG1(stack);
	char *newPath = (char*)SYSC_ARG2(stack);
	if(!sysc_isStringReadable(oldPath) || !sysc_isStringReadable(newPath))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_link(t->tid,oldPath,newPath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_unlink(sIntrptStackFrame *stack) {
	s32 res;
	sThread *t = thread_getRunning();
	char *path = (char*)SYSC_ARG1(stack);
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_unlink(t->tid,path);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_mkdir(sIntrptStackFrame *stack) {
	s32 res;
	sThread *t = thread_getRunning();
	char *path = (char*)SYSC_ARG1(stack);
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_mkdir(t->tid,path);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_rmdir(sIntrptStackFrame *stack) {
	s32 res;
	sThread *t = thread_getRunning();
	char *path = (char*)SYSC_ARG1(stack);
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_rmdir(t->tid,path);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_mount(sIntrptStackFrame *stack) {
	s32 res;
	tInodeNo ino;
	sThread *t = thread_getRunning();
	char *device = (char*)SYSC_ARG1(stack);
	char *path = (char*)SYSC_ARG2(stack);
	u16 type = (u16)SYSC_ARG3(stack);
	if(!sysc_isStringReadable(device) || !sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(vfsn_resolvePath(path,&ino,NULL,VFS_READ) != ERR_REAL_PATH)
		SYSC_ERROR(stack,ERR_MOUNT_VIRT_PATH);

	res = vfsr_mount(t->tid,device,path,type);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_unmount(sIntrptStackFrame *stack) {
	s32 res;
	tInodeNo ino;
	sThread *t = thread_getRunning();
	char *path = (char*)SYSC_ARG1(stack);
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(vfsn_resolvePath(path,&ino,NULL,VFS_READ) != ERR_REAL_PATH)
		SYSC_ERROR(stack,ERR_MOUNT_VIRT_PATH);

	res = vfsr_unmount(t->tid,path);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

bool sysc_isStringReadable(const char *str) {
	u32 addr,rem;
	/* null is a special case */
	if(str == NULL)
		return false;

	/* check if it is readable */
	addr = (u32)str & ~(PAGE_SIZE - 1);
	rem = (addr + PAGE_SIZE) - (u32)str;
	while(paging_isRangeUserReadable(addr,PAGE_SIZE)) {
		while(rem-- > 0 && *str)
			str++;
		if(!*str)
			return true;
		addr += PAGE_SIZE;
		rem = PAGE_SIZE;
	}
	return false;
}
