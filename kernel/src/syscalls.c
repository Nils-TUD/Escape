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
#include <intrpt.h>
#include <proc.h>
#include <vfs.h>
#include <vfsnode.h>
#include <vfsreal.h>
#include <vfsdrv.h>
#include <vfsrw.h>
#include <paging.h>
#include <util.h>
#include <debug.h>
#include <kheap.h>
#include <sched.h>
#include <video.h>
#include <signals.h>
#include <elf.h>
#include <multiboot.h>
#include <timer.h>
#include <text.h>
#include <sharedmem.h>
#include <ioports.h>
#include <gdt.h>
#include <lock.h>
#include <string.h>
#include <assert.h>
#include <errors.h>
#include <messages.h>

/* the max. size we'll allow for exec()-arguments */
#define EXEC_MAX_ARGSIZE				(2 * K)

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
 * Blocks the process until a message arrives
 */
static void sysc_wait(sIntrptStackFrame *stack);
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
static void sysc_getFileInfo(sIntrptStackFrame *stack);
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
 * @return 0 on success
 */
static void sysc_setDataReadable(sIntrptStackFrame *stack);
/**
 * Returns the cpu-cycles for the current thread
 *
 * @return the cpu-cycles
 */
static void sysc_getCycles(sIntrptStackFrame *stack);
/**
 * Writes all dirty objects of the filesystem to disk
 *
 * @return 0 on success
 */
static void sysc_sync(sIntrptStackFrame *stack);

/**
 * Checks wether the given null-terminated string (in user-space) is readable
 *
 * @param char* the string
 * @return true if so
 */
static bool sysc_isStringReadable(const char *string);

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
	/* 29 */	{sysc_getFileInfo,			2},
	/* 30 */	{sysc_debug,				0},
	/* 31 */	{sysc_createSharedMem,		2},
	/* 32 */	{sysc_joinSharedMem,		1},
	/* 33 */	{sysc_leaveSharedMem,		1},
	/* 34 */	{sysc_destroySharedMem,		1},
	/* 35 */	{sysc_getClientThread,		2},
	/* 36 */	{sysc_lock,					1},
	/* 37 */	{sysc_unlock,				1},
	/* 38 */	{sysc_startThread,			0},
	/* 39 */	{sysc_gettid,				0},
	/* 40 */	{sysc_getThreadCount,		0},
	/* 41 */	{sysc_send,					4},
	/* 42 */	{sysc_receive,				3},
	/* 43 */	{sysc_ioctl,				4},
	/* 44 */	{sysc_setDataReadable,		2},
	/* 45 */	{sysc_getCycles,			0},
	/* 46 */	{sysc_sync,					0},
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
		SYSC_ERROR(stack,ERR_MAX_PROCS_REACHED);

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
	s32 res = proc_startThread(entryPoint);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_exit(sIntrptStackFrame *stack) {
	UNUSED(stack);
	/*sThread *t = thread_getRunning();
	if(SYSC_ARG1(stack) != 0) {
		vid_printf("Thread %d (%s) exited with %d\n",t->tid,t->proc->command,SYSC_ARG1(stack));
		util_printStackTrace(util_getUserStackTrace(t,stack));
	}*/
	proc_destroyThread();
	thread_switch();
}

static void sysc_open(sIntrptStackFrame *stack) {
	char *path = (char*)SYSC_ARG1(stack);
	s32 pathLen;
	u8 flags;
	tVFSNodeNo nodeNo = 0;
	tFileNo file;
	tFD fd;
	s32 err;
	sThread *t = thread_getRunning();

	/* at first make sure that we'll cause no page-fault */
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	pathLen = strnlen(path,MAX_PATH_LEN);
	if(pathLen == -1)
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	/* check flags */
	flags = ((u8)SYSC_ARG2(stack)) & (VFS_WRITE | VFS_READ | VFS_CREATE | VFS_CONNECT | VFS_TRUNCATE);
	if((flags & (VFS_READ | VFS_WRITE)) == 0)
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	/* resolve path */
	err = vfsn_resolvePath(path,&nodeNo,flags);
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
		/* open file */
		file = vfs_openFile(t->tid,flags,nodeNo);
		if(file < 0)
			SYSC_ERROR(stack,file);
	}

	/* assoc fd with file */
	err = thread_assocFd(fd,file);
	if(err < 0)
		SYSC_ERROR(stack,err);

	/* if it is a driver, call the driver open-command */
	if(IS_VIRT(nodeNo)) {
		sVFSNode *node = vfsn_getNode(nodeNo);
		if((node->mode & MODE_TYPE_SERVUSE) && (node->parent->mode & MODE_SERVICE_DRIVER)) {
			err = vfsdrv_open(t->tid,file,node,flags);
			/* if this went wrong, undo everything and report an error to the user */
			if(err < 0) {
				thread_unassocFd(fd);
				vfs_closeFile(t->tid,file);
				SYSC_ERROR(stack,err);
			}
		}
	}

	/* return fd */
	SYSC_RET1(stack,fd);
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
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	res = vfs_seek(t->tid,file,offset,whence);
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
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
	if(!paging_isRangeUserWritable((u32)buffer,count))
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

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
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
	if(!paging_isRangeUserReadable((u32)buffer,count))
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

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
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
	/* TODO always writable ok? */
	if(data != NULL && !paging_isRangeUserWritable((u32)data,size))
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

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
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
	if(!paging_isRangeUserReadable((u32)data,size))
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

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
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

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
	if((type & (0x1 | 0x2 | 0x4)) == 0 || (type & ~(0x1 | 0x2 | 0x4)) != 0)
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	/* convert type */
	vtype = 0;
	if(type & 0x2)
		vtype |= MODE_SERVICE_DRIVER;
	else if(type & 0x4)
		vtype |= MODE_SERVICE_FS;

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
	if(!IS_VIRT(id) || !vfsn_isValidNodeNo(id))
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

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
	if(!IS_VIRT(id) || !vfsn_isValidNodeNo(id))
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

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
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
	if(client == NULL || !paging_isRangeUserWritable((u32)client,sizeof(tServ)))
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	/* we need a file-desc */
	fd = thread_getFreeFd();
	if(fd < 0)
		SYSC_ERROR(stack,fd);

	/* open a client */
	file = vfs_openClient(t->tid,(tVFSNodeNo*)ids,count,(tVFSNodeNo*)client);
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
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

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
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

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
	bool canSleep;

	if((events & ~(EV_CLIENT | EV_RECEIVED_MSG | EV_CHILD_DIED | EV_DATA_READABLE)) != 0)
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	/* check wether there is a chance that we'll wake up again */
	canSleep = !vfs_msgAvailableFor(t->tid,events);
	if(canSleep && (events & EV_CHILD_DIED))
		canSleep = proc_hasChild(t->proc->pid);

	/* if we can sleep, do it */
	if(canSleep) {
		thread_wait(t->tid,events);
		thread_switch();
	}
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

	/* check range */
	if(count == 0 || (u32)start + (u32)count > 0xFFFF)
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	sProc *p = proc_getRunning();
	s32 err = ioports_request(p,start,count);
	if(err < 0)
		SYSC_ERROR(stack,err);

	tss_setIOMap(p->ioMap);
	SYSC_RET1(stack,0);
}

static void sysc_releaseIOPorts(sIntrptStackFrame *stack) {
	u16 start = SYSC_ARG1(stack);
	u16 count = SYSC_ARG2(stack);

	/* check range */
	if(count == 0 || (u32)start + (u32)count > 0xFFFF)
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	sProc *p = proc_getRunning();
	s32 err = ioports_release(p,start,count);
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
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

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
	char **arg;
	char *argBuffer;
	u32 argc;
	s32 remaining = EXEC_MAX_ARGSIZE;
	s32 pathLen,res;
	tVFSNodeNo nodeNo;
	sProc *p = proc_getRunning();

	argc = 0;
	argBuffer = NULL;
	if(args != NULL) {
		char *bufPos;
		s32 len;

		/* alloc space for the arguments */
		argBuffer = (char*)kheap_alloc(EXEC_MAX_ARGSIZE);
		if(argBuffer == NULL)
			SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);

		/* count args and copy them on the kernel-heap */
		/* note that we have to create a copy since we don't know where the args are. Maybe
		 * they are on the user-stack at the position we want to copy them for the
		 * new process... */
		bufPos = argBuffer;
		arg = args;
		while(1) {
			/* check if it is a valid pointer */
			if(!paging_isRangeUserReadable((u32)arg,sizeof(char*))) {
				kheap_free(argBuffer);
				SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
			}
			/* end of list? */
			if(*arg == NULL)
				break;

			/* check wether the string is readable */
			if(!sysc_isStringReadable(*arg)) {
				kheap_free(argBuffer);
				SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
			}

			/* ensure that the argument is not longer than the left space */
			len = strnlen(*arg,remaining - 1);
			if(len == -1) {
				/* too long */
				kheap_free(argBuffer);
				SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
			}

			/* copy to heap */
			memcpy(bufPos,*arg,len + 1);
			bufPos += len + 1;
			remaining -= len + 1;
			arg++;
			argc++;
		}
	}

	/* at first make sure that we'll cause no page-fault */
	if(!sysc_isStringReadable(path)) {
		kheap_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
	}

	/* save path (we'll overwrite the process-data) */
	pathLen = strlen(path);
	if(pathLen >= MAX_PATH_LEN) {
		kheap_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
	}
	memcpy(pathSave,path,pathLen + 1);
	path = pathSave;

	/* resolve path; require a path in real fs */
	res = vfsn_resolvePath(path,&nodeNo,VFS_READ);
	if(res != ERR_REAL_PATH) {
		kheap_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
	}

	/* free the current text; free frames if text_free() returns true */
	paging_unmap(0,p->textPages,text_free(p->text,p->pid),false);
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
		proc_destroy(p);
		thread_switch();
		return;
	}

	/* copy path so that we can identify the process */
	memcpy(p->command,pathSave + (pathLen > MAX_PROC_NAME_LEN ? (pathLen - MAX_PROC_NAME_LEN) : 0),
			MIN(MAX_PROC_NAME_LEN,pathLen) + 1);

	/* make process ready */
	proc_setupUserStack(stack,argc,argBuffer,EXEC_MAX_ARGSIZE - remaining);
	proc_setupStart(stack,(u32)res);

	kheap_free(argBuffer);
}

static void sysc_getFileInfo(sIntrptStackFrame *stack) {
	char *path = (char*)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	tVFSNodeNo nodeNo;
	u32 len;
	s32 res;

	if(!sysc_isStringReadable(path) || info == NULL ||
			!paging_isRangeUserWritable((u32)info,sizeof(sFileInfo)))
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
	len = strlen(path);
	if(len == 0 || len >= MAX_PATH_LEN)
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	res = vfsn_resolvePath(path,&nodeNo,VFS_READ);
	if(res == ERR_REAL_PATH) {
		sThread *t = thread_getRunning();
		res = vfsr_getFileInfo(t->tid,path,info);
	}
	else if(res == 0)
		res = vfsn_getNodeInfo(nodeNo,info);

	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

static void sysc_debug(sIntrptStackFrame *stack) {
	UNUSED(stack);
	/*vfsn_dbg_printTree();*/
	paging_dbg_printOwnPageDir(PD_PART_USER);
}

static void sysc_createSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	u32 byteCount = SYSC_ARG2(stack);
	s32 res;

	if(!sysc_isStringReadable(name) || byteCount == 0)
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	res = shm_create(name,BYTES_2_PAGES(byteCount));
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res * PAGE_SIZE);
}

static void sysc_joinSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	s32 res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	res = shm_join(name);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res * PAGE_SIZE);
}

static void sysc_leaveSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	s32 res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

	res = shm_leave(name);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

static void sysc_destroySharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	s32 res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);

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

static bool sysc_isStringReadable(const char *str) {
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
