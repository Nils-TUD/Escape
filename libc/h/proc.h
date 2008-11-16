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
u32 getpid(void);

/**
 * @return the parent-pid
 */
u32 getppid(void);

/**
 * Clones the current process. Will return the pid for the old process and 0 for the new one.
 *
 * @return new pid or 0 or -1 if failed
 */
u16 fork(void);

#endif /* PROC_H_ */
