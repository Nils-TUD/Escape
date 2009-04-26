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
#include <string.h>
#include <assert.h>
#include <errors.h>

/* the max. size we'll allow for exec()-arguments */
#define EXEC_MAX_ARGSIZE				(2 * K)

#define SYSCALL_COUNT					36

/* some convenience-macros */
#define SYSC_ERROR(stack,errorCode)		((stack)->ebx = (errorCode))
#define SYSC_RET1(stack,val)			((stack)->eax = (val))
#define SYSC_RET2(stack,val)			((stack)->edx = (val))
#define SYSC_NUMBER(stack)				((stack)->eax)
#define SYSC_ARG1(stack)				((stack)->ecx)
#define SYSC_ARG2(stack)				((stack)->edx)
#define SYSC_ARG3(stack)				(*((u32*)(stack)->uesp))
#define SYSC_ARG4(stack)				(*((u32*)(stack)->uesp + 1))

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
 * Sets the position for the given file
 *
 * @param tFD the file-descriptor
 * @param u32 the position to set
 * @return 0 on success
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
 * Creates an info-node in the VFS that can be read by other processes
 *
 * @param char* the path
 */
static void sysc_createNode(sIntrptStackFrame *stack);
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
 * Checks wether the given null-terminated string (in user-space) is readable
 *
 * @param char* the string
 * @return true if so
 */
static bool sysc_isStringReadable(const char *string);

/* our syscalls */
static sSyscall syscalls[SYSCALL_COUNT] = {
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
	/* 28 */	{sysc_createNode,			1},
	/* 29 */	{sysc_seek,					2},
	/* 30 */	{sysc_getFileInfo,			2},
	/* 31 */	{sysc_debug,				0},
	/* 32 */	{sysc_createSharedMem,		2},
	/* 33 */	{sysc_joinSharedMem,		1},
	/* 34 */	{sysc_leaveSharedMem,		1},
	/* 35 */	{sysc_destroySharedMem,		1},
};

void sysc_handle(sIntrptStackFrame *stack) {
	u32 sysCallNo = SYSC_NUMBER(stack);
	if(sysCallNo < SYSCALL_COUNT) {
		u32 argCount = syscalls[sysCallNo].argCount;
		u32 ebxSave = stack->ebx;
		/* handle copy-on-write (the first 2 args are passed in registers) */
		/* TODO maybe we need more stack-pages */
		if(argCount > 2)
			paging_isRangeUserWritable((u32)stack->uesp,sizeof(u32) * (argCount - 2));

		/* no error by default */
		SYSC_ERROR(stack,0);
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

static void sysc_getppid(sIntrptStackFrame *stack) {
	tPid pid = (tPid)SYSC_ARG1(stack);
	sProc *p;

	if(!proc_exists(pid)) {
		SYSC_ERROR(stack,ERR_INVALID_PID);
		return;
	}

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
	if(newPid == INVALID_PID) {
		SYSC_ERROR(stack,ERR_MAX_PROCS_REACHED);
		return;
	}

	res = proc_clone(newPid);
	/* error? */
	if(res < 0) {
		SYSC_ERROR(stack,res);
	}
	/* child? */
	else if(res == 1) {
		SYSC_RET1(stack,0);
	}
	/* parent */
	else {
		SYSC_RET1(stack,newPid);
	}
}

static void sysc_exit(sIntrptStackFrame *stack) {
	UNUSED(stack);
	sProc *p = proc_getRunning();
	/*vid_printf("Process %d exited with exit-code %d\n",p->pid,SYSC_ARG1(stack));*/
	proc_destroy(p);
	proc_switch();
}

static void sysc_open(sIntrptStackFrame *stack) {
	char *path = (char*)SYSC_ARG1(stack);
	s32 pathLen;
	u8 flags;
	tVFSNodeNo nodeNo;
	tFileNo file;
	tFD fd;
	s32 err;
	sProc *p = proc_getRunning();

	/* at first make sure that we'll cause no page-fault */
	if(!sysc_isStringReadable(path)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	pathLen = strnlen(path,MAX_PATH_LEN);
	if(pathLen == -1) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* check flags */
	flags = ((u8)SYSC_ARG2(stack)) & (VFS_WRITE | VFS_READ);
	if((flags & (VFS_READ | VFS_WRITE)) == 0) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* resolve path */
	err = vfsn_resolvePath(path,&nodeNo);
	if(err == ERR_REAL_PATH) {
		/* send msg to fs and wait for reply */

		/* skip file: */
		if(strncmp(path,"file:",5) == 0)
			path += 5;
		file = vfsr_openFile(p->pid,flags,path);
		if(file < 0) {
			SYSC_ERROR(stack,file);
			return;
		}

		/* get free fd */
		fd = proc_getFreeFd();
		if(fd < 0) {
			vfs_closeFile(file);
			SYSC_ERROR(stack,fd);
			return;
		}
	}
	else {
		/* handle virtual files */
		if(err < 0) {
			SYSC_ERROR(stack,err);
			return;
		}

		/* get free fd */
		fd = proc_getFreeFd();
		if(fd < 0) {
			SYSC_ERROR(stack,fd);
			return;
		}
		/* open file */
		file = vfs_openFile(p->pid,flags,nodeNo);
	}

	if(file < 0) {
		SYSC_ERROR(stack,file);
		return;
	}

	/* assoc fd with file */
	err = proc_assocFd(fd,file);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	/* return fd */
	SYSC_RET1(stack,fd);
}

static void sysc_eof(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	tFileNo file;
	bool eof;

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0) {
		SYSC_ERROR(stack,file);
		return;
	}

	eof = vfs_eof(p->pid,file);
	SYSC_RET1(stack,eof);
}

static void sysc_seek(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	u32 position = SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	tFileNo file;
	s32 res;

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0) {
		SYSC_ERROR(stack,file);
		return;
	}

	res = vfs_seek(p->pid,file,position);
	SYSC_RET1(stack,res);
}

static void sysc_read(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	void *buffer = (void*)SYSC_ARG2(stack);
	u32 count = SYSC_ARG3(stack);
	sProc *p = proc_getRunning();
	s32 readBytes;
	tFileNo file;

	/* validate count and buffer */
	if(count <= 0) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}
	if(!paging_isRangeUserWritable((u32)buffer,count)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0) {
		SYSC_ERROR(stack,file);
		return;
	}

	/* read */
	readBytes = vfs_readFile(p->pid,file,buffer,count);
	if(readBytes < 0) {
		SYSC_ERROR(stack,readBytes);
		return;
	}

	SYSC_RET1(stack,readBytes);
}

static void sysc_write(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	void *buffer = (void*)SYSC_ARG2(stack);
	u32 count = SYSC_ARG3(stack);
	sProc *p = proc_getRunning();
	s32 writtenBytes;
	tFileNo file;

	/* validate count and buffer */
	if(count <= 0) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}
	if(!paging_isRangeUserReadable((u32)buffer,count)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0) {
		SYSC_ERROR(stack,file);
		return;
	}

	/* read */
	writtenBytes = vfs_writeFile(p->pid,file,buffer,count);
	if(writtenBytes < 0) {
		SYSC_ERROR(stack,writtenBytes);
		return;
	}

	SYSC_RET1(stack,writtenBytes);
}

static void sysc_dupFd(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	tFD res;

	res = proc_dupFd(fd);
	if(res < 0) {
		SYSC_ERROR(stack,res);
		return;
	}

	SYSC_RET1(stack,res);
}

static void sysc_redirFd(sIntrptStackFrame *stack) {
	tFD src = (tFD)SYSC_ARG1(stack);
	tFD dst = (tFD)SYSC_ARG2(stack);
	s32 err;

	err = proc_redirFd(src,dst);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	SYSC_RET1(stack,err);
}

static void sysc_close(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);

	/* unassoc fd */
	tFileNo fileNo = proc_unassocFD(fd);
	if(fileNo < 0)
		return;

	/* close file */
	vfs_closeFile(fileNo);
}

static void sysc_regService(sIntrptStackFrame *stack) {
	const char *name = (const char*)SYSC_ARG1(stack);
	u32 type = (u32)SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	tServ res;

	/* check type */
	if((type & (0x1 | 0x2)) == 0 || (type & ~(0x1 | 0x2)) != 0) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* convert type */
	if(type & 0x2)
		type = MODE_SERVICE_SINGLEPIPE;
	else
		type = 0;

	res = vfs_createService(p->pid,name,type);
	if(res < 0) {
		SYSC_ERROR(stack,res);
		return;
	}

	SYSC_RET1(stack,res);
}

static void sysc_unregService(sIntrptStackFrame *stack) {
	tServ id = SYSC_ARG1(stack);
	s32 err;

	/* check node-number */
	if(!IS_VIRT(id) || !vfsn_isValidNodeNo(id)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* remove the service */
	err = vfs_removeService(proc_getRunning()->pid,id);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	SYSC_RET1(stack,0);
}

static void sysc_getClient(sIntrptStackFrame *stack) {
	tServ *ids = (tServ*)SYSC_ARG1(stack);
	u32 count = SYSC_ARG2(stack);
	tServ *client = (tServ*)SYSC_ARG3(stack);
	sProc *p = proc_getRunning();
	tFD fd;
	s32 res;
	tFileNo file;

	/* check arguments. limit count a little bit to prevent overflow */
	if(count <= 0 || count > 32 || ids == NULL ||
			!paging_isRangeUserReadable((u32)ids,count * sizeof(tServ))) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}
	if(client == NULL || !paging_isRangeUserWritable((u32)client,sizeof(tServ))) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* we need a file-desc */
	fd = proc_getFreeFd();
	if(fd < 0) {
		SYSC_ERROR(stack,fd);
		return;
	}

	/* open a client */
	file = vfs_openClient(p->pid,(tVFSNodeNo*)ids,count,(tVFSNodeNo*)client);
	if(file < 0) {
		SYSC_ERROR(stack,file);
		return;
	}

	/* associate fd with file */
	res = proc_assocFd(fd,file);
	if(res < 0) {
		/* we have already opened the file */
		vfs_closeFile(file);
		SYSC_ERROR(stack,res);
		return;
	}

	SYSC_RET1(stack,fd);
}

static void sysc_changeSize(sIntrptStackFrame *stack) {
	s32 count = SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	u32 oldEnd = p->textPages + p->dataPages;

	/* check argument */
	if(count > 0) {
		if(!proc_segSizesValid(p->textPages,p->dataPages + count,p->stackPages)) {
			SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
			return;
		}
	}
	else if(count == 0) {
		SYSC_RET1(stack,oldEnd);
		return;
	}
	else if((u32)-count > p->dataPages) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* change size */
	if(proc_changeSize(count,CHG_DATA) == false) {
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
		return;
	}

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
	/*if(phys > KERNEL_P_ADDR || phys + pages * PAGE_SIZE > KERNEL_P_ADDR) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}*/

	/* determine start-address */
	addr = (p->textPages + p->dataPages) * PAGE_SIZE;
	origPages = p->textPages + p->dataPages;

	/* not enough mem? */
	if(mm_getFreeFrmCount(MM_DEF) < paging_countFramesForMap(addr,pages)) {
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
		return;
	}

	/* invalid segment sizes? */
	if(!proc_segSizesValid(p->textPages,p->dataPages + pages,p->stackPages)) {
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
		return;
	}

	/* we have to allocate temporary space for the frame-address :( */
	frames = kheap_alloc(sizeof(u32) * pages);
	if(frames == NULL) {
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
		return;
	}

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
	sProc *p;
	bool canSleep;

	if((events & ~(EV_CLIENT | EV_RECEIVED_MSG | EV_CHILD_DIED)) != 0) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	p = proc_getRunning();

	/* TODO we need a result for EV_CHILD_DIED (and maybe for others, too) */

	/* check wether there is a chance that we'll wake up again */
	canSleep = !vfs_msgAvailableFor(p->pid,events);
	if(canSleep && (events & EV_CHILD_DIED))
		canSleep = proc_hasChild(p->pid);

	/* if we can sleep, do it */
	if(canSleep) {
		proc_wait(p->pid,events);
		proc_switch();
	}
}

static void sysc_sleep(sIntrptStackFrame *stack) {
	u32 msecs = SYSC_ARG1(stack);
	timer_sleepFor(proc_getRunning()->pid,msecs);
	proc_switch();
}

static void sysc_yield(sIntrptStackFrame *stack) {
	UNUSED(stack);

	proc_switch();
}

static void sysc_requestIOPorts(sIntrptStackFrame *stack) {
	u16 start = SYSC_ARG1(stack);
	u16 count = SYSC_ARG2(stack);

	/* check range */
	if(count == 0 || (u32)start + (u32)count > 0xFFFF) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	s32 err = proc_requestIOPorts(start,count);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	SYSC_RET1(stack,0);
}

static void sysc_releaseIOPorts(sIntrptStackFrame *stack) {
	u16 start = SYSC_ARG1(stack);
	u16 count = SYSC_ARG2(stack);

	/* check range */
	if(count == 0 || (u32)start + (u32)count > 0xFFFF) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	s32 err = proc_releaseIOPorts(start,count);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	SYSC_RET1(stack,0);
}

static void sysc_setSigHandler(sIntrptStackFrame *stack) {
	tSig signal = (tSig)SYSC_ARG1(stack);
	fSigHandler handler = (fSigHandler)SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	s32 err;

	/* address should be valid */
	if(!paging_isRangeUserReadable((u32)handler,1)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* check signal */
	if(!sig_canHandle(signal)) {
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);
		return;
	}

	err = sig_setHandler(p->pid,signal,handler);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	SYSC_RET1(stack,err);
}

static void sysc_unsetSigHandler(sIntrptStackFrame *stack) {
	tSig signal = (tSig)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();

	if(!sig_canHandle(signal)) {
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);
		return;
	}

	sig_unsetHandler(p->pid,signal);

	SYSC_RET1(stack,0);
}

static void sysc_ackSignal(sIntrptStackFrame *stack) {
	sProc *p = proc_getRunning();
	sig_ackHandling(p->pid);
	SYSC_RET1(stack,0);
}

static void sysc_sendSignalTo(sIntrptStackFrame *stack) {
	tPid pid = (tPid)SYSC_ARG1(stack);
	tSig signal = (tSig)SYSC_ARG2(stack);
	u32 data = SYSC_ARG3(stack);
	sProc *p = proc_getRunning();

	if(!sig_canSend(signal)) {
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);
		return;
	}

	if(pid != INVALID_PID && !proc_exists(pid)) {
		SYSC_ERROR(stack,ERR_INVALID_PID);
		return;
	}

	if(pid != INVALID_PID)
		sig_addSignalFor(pid,signal,data);
	else
		sig_addSignal(signal,data);

	/* choose another process if we've killed ourself */
	if(p->state != ST_RUNNING)
		proc_switch();

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
		argBuffer = kheap_alloc(EXEC_MAX_ARGSIZE);
		if(argBuffer == NULL) {
			SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
			return;
		}

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
				return;
			}
			/* end of list? */
			if(*arg == NULL)
				break;

			/* check wether the string is readable */
			if(!sysc_isStringReadable(*arg)) {
				kheap_free(argBuffer);
				SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
				return;
			}

			/* ensure that the argument is not longer than the left space */
			len = strnlen(*arg,remaining - 1);
			if(len == -1) {
				/* too long */
				kheap_free(argBuffer);
				SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
				return;
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
		return;
	}

	/* save path (we'll overwrite the process-data) */
	pathLen = strlen(path);
	if(pathLen >= MAX_PATH_LEN) {
		kheap_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}
	memcpy(pathSave,path,pathLen + 1);
	path = pathSave;

	/* resolve path */
	res = vfsn_resolvePath(path,&nodeNo);
	if(res != ERR_REAL_PATH) {
		kheap_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
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
	if(strncmp(path,"file:",5) == 0)
		path += 5;
	res = elf_loadFromFile(path);
	if(res < 0) {
		/* there is no undo for proc_changeSize() :/ */
		kheap_free(argBuffer);
		proc_destroy(p);
		proc_switch();
		return;
	}

	/* copy path so that we can identify the process */
	memcpy(p->command,pathSave + (pathLen > MAX_PROC_NAME_LEN ? (pathLen - MAX_PROC_NAME_LEN) : 0),
			MIN(MAX_PROC_NAME_LEN,pathLen) + 1);

	/* make process ready */
	proc_setupIntrptStack(intrpt_getCurStack(),argc,argBuffer,EXEC_MAX_ARGSIZE - remaining);
	kheap_free(argBuffer);
}

static void sysc_createNode(sIntrptStackFrame *stack) {
	const char *path = (char*)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	u32 nameLen,pathLen;
	s32 res;
	tVFSNodeNo nodeNo,dummyNodeNo;
	sVFSNode *node;
	char *name,*pathCpy,*nameCpy;

	/* at first make sure that we'll cause no page-fault */
	if(!sysc_isStringReadable(path)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	pathLen = strlen(path);
	if(pathLen == 0 || pathLen >= MAX_PATH_LEN) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* determine last slash */
	name = strrchr(path,'/');
	if(*(name + 1) == '\0')
		name = strrchr(name - 1,'/');
	/* skip '/' */
	name++;
	/* create a copy of the path */
	nameLen = strlen(name);
	pathLen -= nameLen;
	pathCpy = (char*)kheap_alloc(pathLen + 1);
	if(pathCpy == NULL) {
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
		return;
	}
	strncpy(pathCpy,path,pathLen);
	pathCpy[pathLen] = '\0';

	/* make a copy of the name */
	nameCpy = (char*)kheap_alloc(nameLen + 1);
	if(nameCpy == NULL) {
		kheap_free(pathCpy);
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
		return;
	}
	strcpy(nameCpy,name);

	/* resolve path */
	res = vfsn_resolvePath(pathCpy,&nodeNo);
	if(res < 0) {
		kheap_free(pathCpy);
		SYSC_ERROR(stack,res);
		return;
	}
	/* check wether the node does already exist */
	res = vfsn_resolvePath(path,&dummyNodeNo);
	if(res >= 0 || res == ERR_REAL_PATH) {
		SYSC_ERROR(stack,ERR_NODE_EXISTS);
		return;
	}

	/* create node */
	kheap_free(pathCpy);
	node = vfsn_getNode(nodeNo);
	if(vfsn_createInfo(p->pid,node,nameCpy,vfs_defReadHandler) == NULL) {
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
		return;
	}

	SYSC_RET1(stack,0);
}

static void sysc_getFileInfo(sIntrptStackFrame *stack) {
	char *path = (char*)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	tVFSNodeNo nodeNo;
	u32 len;
	s32 res;

	if(!sysc_isStringReadable(path) || info == NULL ||
			!paging_isRangeUserWritable((u32)info,sizeof(sFileInfo))) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}
	len = strlen(path);
	if(len == 0 || len >= MAX_PATH_LEN) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	res = vfsn_resolvePath(path,&nodeNo);
	if(res == ERR_REAL_PATH) {
		sProc *p = proc_getRunning();
		/* skip file: */
		if(strncmp(path,"file:",5) == 0)
			path += 5;
		res = vfsr_getFileInfo(p->pid,path,info);
	}
	else if(res == 0)
		res = vfsn_getNodeInfo(nodeNo,info);

	if(res < 0) {
		SYSC_ERROR(stack,res);
		return;
	}
	SYSC_RET1(stack,0);
}

static void sysc_debug(sIntrptStackFrame *stack) {
	UNUSED(stack);
	proc_dbg_print(proc_getRunning());
}

static void sysc_createSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	u32 byteCount = SYSC_ARG2(stack);
	s32 res;

	if(!sysc_isStringReadable(name) || byteCount == 0) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	res = shm_create(name,BYTES_2_PAGES(byteCount));
	if(res < 0) {
		SYSC_ERROR(stack,res);
		return;
	}

	shm_dbg_print();
	SYSC_RET1(stack,res * PAGE_SIZE);
}

static void sysc_joinSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	s32 res;

	if(!sysc_isStringReadable(name)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	res = shm_join(name);
	if(res < 0) {
		SYSC_ERROR(stack,res);
		return;
	}

	shm_dbg_print();
	SYSC_RET1(stack,res * PAGE_SIZE);
}

static void sysc_leaveSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	s32 res;

	if(!sysc_isStringReadable(name)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	res = shm_leave(name);
	if(res < 0) {
		SYSC_ERROR(stack,res);
		return;
	}

	shm_dbg_print();
	SYSC_RET1(stack,res);
}

static void sysc_destroySharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	s32 res;

	if(!sysc_isStringReadable(name)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	res = shm_destroy(name);
	if(res < 0) {
		SYSC_ERROR(stack,res);
		return;
	}

	shm_dbg_print();
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
