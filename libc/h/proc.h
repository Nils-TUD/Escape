/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PROC_H_
#define PROC_H_

#define MAX_PROC_NAME_LEN	15

/* the events we can wait for */
#define EV_NOEVENT			0
#define EV_CLIENT			1
#define EV_RECEIVED_MSG		2

/* process-data */
typedef struct {
	u8 state;
	tPid pid;
	tPid parentPid;
	u32 textPages;
	u32 dataPages;
	u32 stackPages;
	u64 cycleCount;
	s8 name[MAX_PROC_NAME_LEN + 1];
} sProc;

/* the process states */
typedef enum {ST_UNUSED = 0,ST_RUNNING = 1,ST_READY = 2,ST_BLOCKED = 3,ST_ZOMBIE = 4} eProcState;

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
 * Exchanges the process-data with the given program
 *
 * @param path the program-path
 */
void exec(string path);

/**
 * Releases the CPU (reschedule)
 */
void yield(void);

/**
 * Puts the process to sleep until one of the given events occurrs. Note that you will
 * always be waked up for signals!
 *
 * @param events the events on which you want to wake up
 */
void sleep(u8 events);

/**
 * Destroys the process and provides the parent the given error-code
 *
 * @param errorCode the error-code for the parent
 */
void exit(u32 errorCode);

#endif /* PROC_H_ */
