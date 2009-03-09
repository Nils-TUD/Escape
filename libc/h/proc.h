/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PROC_H_
#define PROC_H_

/* process-data */
typedef struct {
	u8 state;
	tPid pid;
	tPid parentPid;
	u32 textPages;
	u32 dataPages;
	u32 stackPages;
	u64 cycleCount;
} sProc;

/**
 * @return the process-id
 */
tPid getpid(void);

/**
 * @return the parent-pid
 */
tPid getppid(void);

/**
 * Clones the current process. Will return the pid for the old process and 0 for the new one.
 *
 * @return new pid or 0 or -1 if failed
 */
s32 fork(void);

/**
 * Releases the CPU (reschedule)
 */
void yield(void);

/**
 * Puts the process to sleep until a message arrives. That means if the process has no open
 * file and is no service that waits for clients the process will sleep forever!
 * Note also that the process HAS TO check all files for messages before he goes to sleep.
 * Otherwise there might be a message, the process goes to sleep and will he not be waked up
 * until another message arrives.
 */
void sleep(void);

/**
 * Destroys the process and provides the parent the given error-code
 *
 * @param errorCode the error-code for the parent
 */
void exit(u32 errorCode);

#endif /* PROC_H_ */
