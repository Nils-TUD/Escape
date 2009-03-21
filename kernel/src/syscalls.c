/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/syscalls.h"
#include "../h/intrpt.h"
#include "../h/proc.h"
#include "../h/vfs.h"
#include "../h/vfsnode.h"
#include "../h/vfsreal.h"
#include "../h/paging.h"
#include "../h/util.h"
#include "../h/debug.h"
#include "../h/kheap.h"
#include "../h/sched.h"
#include "../h/video.h"
#include "../h/signals.h"
#include "../h/elf.h"
#include <string.h>

/* the max. size we'll allow for exec()-arguments */
#define EXEC_MAX_ARGSIZE				(2 * K)

#define SYSCALL_COUNT					27

/* some convenience-macros */
#define SYSC_ERROR(stack,errorCode)		((stack)->number = (errorCode))
#define SYSC_RET1(stack,val)			((stack)->arg1 = (val))
#define SYSC_RET2(stack,val)			((stack)->arg2 = (val))

/* syscall-handlers */
typedef void (*fSyscall)(sSysCallStack *stack);

/* for syscall-definitions */
typedef struct {
	fSyscall handler;
	u8 argCount;
} sSyscall;

/**
 * Returns the pid of the current process
 *
 * @return tPid the pid
 */
static void sysc_getpid(sSysCallStack *stack);
/**
 * Returns the parent-pid of the current process
 *
 * @return tPid the parent-pid
 */
static void sysc_getppid(sSysCallStack *stack);
/**
 * Temporary syscall to print out a character
 */
static void sysc_debugc(sSysCallStack *stack);
/**
 * Clones the current process
 *
 * @return tPid 0 for the child, the child-pid for the parent-process
 */
static void sysc_fork(sSysCallStack *stack);
/**
 * Destroys the process and issues a context-switch
 *
 * @param u32 the exit-code
 */
static void sysc_exit(sSysCallStack *stack);
/**
 * Open-syscall. Opens a given path with given mode and returns the file-descriptor to use
 * to the user.
 *
 * @param cstring the vfs-filename
 * @param u32 flags (read / write)
 * @return tFD the file-descriptor
 */
static void sysc_open(sSysCallStack *stack);
/**
 * Read-syscall. Reads a given number of bytes in a given file at the current position
 *
 * @param tFD file-descriptor
 * @param void* buffer
 * @param u32 number of bytes
 * @return u32 the number of read bytes
 */
static void sysc_read(sSysCallStack *stack);
/**
 * Write-syscall. Writes a given number of bytes to a given file at the current position
 *
 * @param tFD file-descriptor
 * @param void* buffer
 * @param u32 number of bytes
 * @return u32 the number of written bytes
 */
static void sysc_write(sSysCallStack *stack);
/**
 * Duplicates the given file-descriptor
 *
 * @param tFD the file-descriptor
 * @return s32 the error-code or the new file-descriptor
 */
static void sysc_dupFd(sSysCallStack *stack);
/**
 * Redirects <src> to <dst>. <src> will be closed. Note that both fds have to exist!
 *
 * @param tFD src the source-file-descriptor
 * @param tFD dst the destination-file-descriptor
 * @return s32 the error-code or 0 if successfull
 */
static void sysc_redirFd(sSysCallStack *stack);
/**
 * Closes the given file-descriptor
 *
 * @param tFD the file-descriptor
 */
static void sysc_close(sSysCallStack *stack);
/**
 * Registers a service
 *
 * @param cstring name of the service
 * @param u8 type: SINGLE-PIPE OR MULTI-PIPE
 * @return s32 0 if successfull
 */
static void sysc_regService(sSysCallStack *stack);
/**
 * Unregisters a service
 *
 * @param s32 the node-id
 * @return s32 0 on success or a negative error-code
 */
static void sysc_unregService(sSysCallStack *stack);
/**
 * For services: Looks wether a client wants to be served and returns a file-descriptor
 * for it.
 *
 * @param sVFSNode* node the service-node
 * @return tFD the file-descriptor to use
 */
static void sysc_getClient(sSysCallStack *stack);
/**
 * Adds an interrupt-listener (for services)
 *
 * @param s32 the node-id
 * @param u16 the irq-number
 * @param void* the message
 * @param u32 the message-length
 * @return s32 0 on success or a negative error-code
 */
static void sysc_addIntrptListener(sSysCallStack *stack);
/**
 * Removes an interrupt-listener (for services)
 *
 * @param s32 the node-id
 * @param u16 the irq-number
 * @return s32 0 on success or a negative error-code
 */
static void sysc_remIntrptListener(sSysCallStack *stack);
/**
 * Changes the process-size
 *
 * @param u32 number of pages
 * @return u32 the previous number of text+data pages
 */
static void sysc_changeSize(sSysCallStack *stack);
/**
 * Maps physical memory in the virtual user-space
 *
 * @param u32 physical address
 * @param u32 number of bytes
 * @return void* the start-address
 */
static void sysc_mapPhysical(sSysCallStack *stack);
/**
 * Blocks the process until a message arrives
 */
static void sysc_sleep(sSysCallStack *stack);
/**
 * Releases the CPU (reschedule)
 */
static void sysc_yield(sSysCallStack *stack);
/**
 * Requests some IO-ports
 *
 * @param u16 start-port
 * @param u16 number of ports
 * @return s32 0 if successfull or a negative error-code
 */
static void sysc_requestIOPorts(sSysCallStack *stack);
/**
 * Releases some IO-ports
 *
 * @param u16 start-port
 * @param u16 number of ports
 * @return s32 0 if successfull or a negative error-code
 */
static void sysc_releaseIOPorts(sSysCallStack *stack);
/**
 * Sets a handler-function for a specific signal
 *
 * @param tSig signal
 * @param fSigHandler handler
 * @return s32 0 if no error
 */
static void sysc_setSigHandler(sSysCallStack *stack);
/**
 * Unsets the handler-function for a specific signal
 *
 * @param tSig signal
 * @return s32 0 if no error
 */
static void sysc_unsetSigHandler(sSysCallStack *stack);
/**
 * Acknoledges that the processing of a signal is finished
 *
 * @param tSig signal
 * @return s32 0 if no error
 */
static void sysc_ackSignal(sSysCallStack *stack);
/**
 * Sends a signal to a process
 *
 * @param tPid pid
 * @param tSig signal
 * @return s32 0 if no error
 */
static void sysc_sendSignal(sSysCallStack *stack);
/**
 * Exchanges the process-data with another program
 *
 * @param string the program-path
 */
static void sysc_exec(sSysCallStack *stack);

/**
 * Checks wether the given null-terminated string (in user-space) is readable
 *
 * @param string the string
 * @return true if so
 */
static bool sysc_isStringReadable(s8 *string);

/* our syscalls */
static sSyscall syscalls[SYSCALL_COUNT] = {
	/* 0 */		{sysc_getpid,				0},
	/* 1 */		{sysc_getppid,				0},
	/* 2 */ 	{sysc_debugc,				1},
	/* 3 */		{sysc_fork,					0},
	/* 4 */ 	{sysc_exit,					1},
	/* 5 */ 	{sysc_open,					2},
	/* 6 */ 	{sysc_close,				1},
	/* 7 */ 	{sysc_read,					3},
	/* 8 */		{sysc_regService,			2},
	/* 9 */ 	{sysc_unregService,			0},
	/* 10 */	{sysc_changeSize,			1},
	/* 11 */	{sysc_mapPhysical,			2},
	/* 12 */	{sysc_write,				3},
	/* 13 */	{sysc_yield,				0},
	/* 14 */	{sysc_getClient,			1},
	/* 15 */	{sysc_requestIOPorts,		2},
	/* 16 */	{sysc_releaseIOPorts,		2},
	/* 17 */	{sysc_dupFd,				1},
	/* 18 */	{sysc_redirFd,				2},
	/* 19 */	{sysc_addIntrptListener,	4},
	/* 20 */	{sysc_remIntrptListener,	2},
	/* 21 */	{sysc_sleep,				0},
	/* 22 */	{sysc_setSigHandler,		2},
	/* 23 */	{sysc_unsetSigHandler,		1},
	/* 24 */	{sysc_ackSignal,			1},
	/* 25 */	{sysc_sendSignal,			2},
	/* 26 */	{sysc_exec,					1},
};

void sysc_handle(sIntrptStackFrame *intrptStack) {
	sSysCallStack *stack = (sSysCallStack*)intrptStack->uesp;
	ASSERT(stack != NULL,"stack == NULL");

	u32 sysCallNo = (u32)stack->number;
	if(sysCallNo < SYSCALL_COUNT) {
		/* no error by default */
		SYSC_ERROR(stack,0);
		syscalls[sysCallNo].handler(stack);
	}
}

static void sysc_getpid(sSysCallStack *stack) {
	sProc *p = proc_getRunning();
	SYSC_RET1(stack,p->pid);
}

static void sysc_getppid(sSysCallStack *stack) {
	sProc *p = proc_getRunning();
	SYSC_RET1(stack,p->parentPid);
}

static void sysc_debugc(sSysCallStack *stack) {
	vid_putchar((s8)stack->arg1);
}

static void sysc_fork(sSysCallStack *stack) {
	tPid newPid = proc_getFreePid();
	s32 res = proc_clone(newPid);

	/* error? */
	if(res == -1) {
		SYSC_RET1(stack,-1);
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

static void sysc_exit(sSysCallStack *stack) {
	sProc *p = proc_getRunning();
	/*vid_printf("Process %d exited with exit-code %d\n",p->pid,stack->arg1);*/
	proc_destroy(p);
	proc_switch();
}

static void sysc_open(sSysCallStack *stack) {
	s8 *path = (s8*)stack->arg1;
	s32 pathLen;
	u8 flags;
	tVFSNodeNo nodeNo;
	tFile file;
	s32 err,fd;
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
	flags = ((u8)stack->arg2) & (VFS_WRITE | VFS_READ);
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

		/* get free fd */
		fd = proc_getFreeFd();
		if(fd < 0) {
			/* TODO clean up */
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

static void sysc_read(sSysCallStack *stack) {
	tFD fd = (tFD)stack->arg1;
	void *buffer = (void*)stack->arg2;
	u32 count = stack->arg3;
	sProc *p = proc_getRunning();
	s32 readBytes;
	tFile file;

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

static void sysc_write(sSysCallStack *stack) {
	tFD fd = (tFD)stack->arg1;
	void *buffer = (void*)stack->arg2;
	u32 count = stack->arg3;
	sProc *p = proc_getRunning();
	s32 writtenBytes;
	tFile file;

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

static void sysc_dupFd(sSysCallStack *stack) {
	tFD fd = (tFD)stack->arg1;
	s32 err;

	err = proc_dupFd(fd);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	SYSC_RET1(stack,err);
}

static void sysc_redirFd(sSysCallStack *stack) {
	tFD src = (tFD)stack->arg1;
	tFD dst = (tFD)stack->arg2;
	s32 err;

	err = proc_redirFd(src,dst);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	SYSC_RET1(stack,err);
}

static void sysc_close(sSysCallStack *stack) {
	tFD fd = (tFD)stack->arg1;

	/* unassoc fd */
	tFile fileNo = proc_unassocFD(fd);
	if(fileNo < 0)
		return;

	/* close file */
	vfs_closeFile(fileNo);
}

static void sysc_regService(sSysCallStack *stack) {
	cstring name = (cstring)stack->arg1;
	u8 type = (u8)stack->arg2;
	sProc *p = proc_getRunning();
	s32 res;

	res = vfs_createService(p->pid,name,type);
	if(res < 0) {
		SYSC_ERROR(stack,res);
		return;
	}

	/*vfs_dbg_printTree();*/

	SYSC_RET1(stack,res);
}

static void sysc_unregService(sSysCallStack *stack) {
	tVFSNodeNo no = stack->arg1;
	s32 err;

	/* check node-number */
	if(!vfsn_isValidNodeNo(no)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* remove the service */
	err = vfs_removeService(proc_getRunning()->pid,no);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	SYSC_RET1(stack,0);
}

static void sysc_getClient(sSysCallStack *stack) {
	tVFSNodeNo no = stack->arg1;
	sProc *p = proc_getRunning();
	s32 fd,res;
	tFile file;

	/* check argument */
	if(!vfsn_isValidNodeNo(no)) {
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
	file = vfs_openClient(p->pid,no);
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

static void sysc_addIntrptListener(sSysCallStack *stack) {
	s32 id = (s32)stack->arg1;
	u16 irq = (u16)stack->arg2;
	void *msg = (void*)stack->arg3;
	u32 msgLen = stack->arg4;
	s32 err;

	/* check id */
	if(!vfsn_isValidNodeNo(id)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	if(!vfsn_isOwnServiceNode((tVFSNodeNo)id)) {
		SYSC_ERROR(stack,ERR_NOT_OWN_SERVICE);
		return;
	}

	/* check msg */
	if(msgLen == 0 || !paging_isRangeUserReadable((u32)msg,msgLen)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* now add the listener */
	err = intrpt_addListener(irq,(tVFSNodeNo)id,msg,msgLen);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	SYSC_RET1(stack,err);
}

static void sysc_remIntrptListener(sSysCallStack *stack) {
	s32 id = (s32)stack->arg1;
	u16 irq = (u16)stack->arg2;
	s32 err;

	/* check id */
	if(!vfsn_isValidNodeNo(id)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	if(!vfsn_isOwnServiceNode((tVFSNodeNo)id)) {
		SYSC_ERROR(stack,ERR_NOT_OWN_SERVICE);
		return;
	}

	/* now remove the listener */
	err = intrpt_removeListener(irq,(tVFSNodeNo)id);
	if(err < 0) {
		SYSC_ERROR(stack,err);
		return;
	}

	SYSC_RET1(stack,err);
}

static void sysc_changeSize(sSysCallStack *stack) {
	s32 count = stack->arg1;
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
	else if((unsigned)-count > p->dataPages) {
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

static void sysc_mapPhysical(sSysCallStack *stack) {
	u32 addr,origPages;
	u32 phys = stack->arg1;
	u32 pages = BYTES_2_PAGES(stack->arg2);
	sProc *p = proc_getRunning();
	u32 i,*frames;

	/* trying to map memory in kernel area? */
	/* TODO is this ok? */
	if(phys + pages * PAGE_SIZE > KERNEL_P_ADDR) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

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
	paging_map(addr,frames,pages,PG_WRITABLE,false);
	kheap_free(frames);

	/* increase datapages */
	p->dataPages += pages;
	/* return start-addr */
	SYSC_RET1(stack,addr);
}

static void sysc_yield(sSysCallStack *stack) {
	UNUSED(stack);

	proc_switch();
}

static void sysc_sleep(sSysCallStack *stack) {
	u8 events = (u8)stack->arg1;
	sProc *p;
	bool msgAv;

	if((events & ~(EV_CLIENT | EV_RECEIVED_MSG | EV_CHILD_DIED)) != 0) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	p = proc_getRunning();

	/* TODO we need a result for EV_CHILD_DIED (and maybe for others, too) */

	msgAv = vfs_msgAvailableFor(p->pid,events);
	if(!msgAv) {
		proc_sleep(events);
		proc_switch();
	}
}

static void sysc_requestIOPorts(sSysCallStack *stack) {
	u16 start = stack->arg1;
	u16 count = stack->arg2;

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

static void sysc_releaseIOPorts(sSysCallStack *stack) {
	u16 start = stack->arg1;
	u16 count = stack->arg2;

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

static void sysc_setSigHandler(sSysCallStack *stack) {
	tSig signal = (tSig)stack->arg1;
	fSigHandler handler = (fSigHandler)stack->arg2;
	sProc *p = proc_getRunning();
	s32 err;

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

static void sysc_unsetSigHandler(sSysCallStack *stack) {
	tSig signal = (tSig)stack->arg1;
	sProc *p = proc_getRunning();

	if(!sig_canHandle(signal)) {
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);
		return;
	}

	sig_unsetHandler(p->pid,signal);

	SYSC_RET1(stack,0);
}

static void sysc_ackSignal(sSysCallStack *stack) {
	tSig signal = (tSig)stack->arg1;
	sProc *p = proc_getRunning();

	if(!sig_canHandle(signal)) {
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);
		return;
	}

	sig_ackHandling(p->pid,signal);
	SYSC_RET1(stack,0);
}

static void sysc_sendSignal(sSysCallStack *stack) {
	tPid pid = (tPid)stack->arg1;
	tSig signal = (tSig)stack->arg2;

	if(sig_isInterrupt(signal) || signal >= SIG_COUNT) {
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);
		return;
	}

	if(!proc_exists(pid)) {
		SYSC_ERROR(stack,ERR_INVALID_PID);
		return;
	}

	sig_addSignalFor(pid,signal);
	SYSC_RET1(stack,0);
}

static void sysc_exec(sSysCallStack *stack) {
	const u32 bytesPerReq = 1 * K;
	s8 pathSave[MAX_PATH_LEN + 1];
	s8 *path = (s8*)stack->arg1;
	s8 **args = (s8**)stack->arg2;
	s8 **arg;
	s8 *argBuffer;
	u8 *buffer;
	u32 argc;
	u32 fileSize,bufSize;
	s32 pathLen,readBytes;
	s32 res;
	tVFSNodeNo nodeNo;
	tFile file;
	sProc *p = proc_getRunning();

	argc = 0;
	argBuffer = NULL;
	if(args != NULL) {
		s8 *bufPos;
		s32 remaining = EXEC_MAX_ARGSIZE;
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
		while(*arg != NULL) {
			/* at first we have to check wether the string is readable */
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

	pathLen = strnlen(path,MAX_PATH_LEN);
	if(pathLen == -1 || !paging_isRangeUserReadable((u32)path,pathLen)) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}

	/* save path (we'll overwrite the process-data) */
	memcpy(pathSave,path,pathLen + 1);

	/* open file */
	res = vfsn_resolvePath(path,&nodeNo);
	if(res != ERR_REAL_PATH) {
		SYSC_ERROR(stack,ERR_INVALID_SYSC_ARGS);
		return;
	}
	/* skip file: */
	if(strncmp(path,"file:",5) == 0)
		path += 5;
	file = vfsr_openFile(p->pid,VFS_READ,path);
	if(file < 0) {
		SYSC_ERROR(stack,file);
		return;
	}

	fileSize = 0;
	bufSize = bytesPerReq * sizeof(u8);
	buffer = (u8*)kheap_alloc(bufSize);
	if(buffer == NULL) {
		vfs_closeFile(file);
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
		return;
	}

	/* read the file */
	do {
		readBytes = vfs_readFile(p->pid,file,buffer + fileSize,bytesPerReq);
		if(readBytes > 0) {
			bufSize += bytesPerReq * sizeof(u8);
			buffer = (u8*)kheap_realloc(buffer,bufSize);
			if(buffer == NULL) {
				vfs_closeFile(file);
				SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
				return;
			}
			fileSize += readBytes;
		}
	}
	while(readBytes > 0);

	/* we don't need the file anymore */
	vfs_closeFile(file);

	/* error? */
	if(readBytes < 0) {
		SYSC_ERROR(stack,readBytes);
		return;
	}

	/* now remove the process-data */
	proc_changeSize(-p->dataPages,CHG_DATA);
	/* copy path so that we can identify the process */
	memcpy(p->command,pathSave + (pathLen > MAX_PROC_NAME_LEN ? (pathLen - MAX_PROC_NAME_LEN) : 0),
			MIN(MAX_PROC_NAME_LEN,pathLen) + 1);

	/* load program and prepare interrupt-stack */
	if(elf_loadprog(buffer) == ELF_INVALID_ENTRYPOINT) {
		/* there is no undo for proc_changeSize() :/ */
		proc_destroy(p);
		if(argBuffer != NULL)
			kheap_free(argBuffer);
		kheap_free(buffer);
		proc_switch();
	}
	else {
		/*vid_printf("%d is finished with '%s'\n",p->pid,pathSave);*/
		proc_setupIntrptStack(intrpt_getCurStack(),argc,argBuffer);

		/* finally free the buffer */
		if(argBuffer != NULL)
			kheap_free(argBuffer);
		kheap_free(buffer);
	}
}

static bool sysc_isStringReadable(s8 *string) {
	u32 addr = (u32)string & ~(PAGE_SIZE - 1);
	u32 rem = (addr + PAGE_SIZE) - (u32)string;
	while(paging_isRangeUserReadable(addr,PAGE_SIZE)) {
		while(rem-- > 0 && *string)
			string++;
		if(!*string)
			return true;
		addr += PAGE_SIZE;
		rem = PAGE_SIZE;
	}
	return false;
}
