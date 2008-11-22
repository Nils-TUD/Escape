/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PROC_H_
#define PROC_H_

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
 * Destroys the process and provides the parent the given error-code
 *
 * @param errorCode the error-code for the parent
 */
void exit(u32 errorCode);

#endif /* PROC_H_ */
