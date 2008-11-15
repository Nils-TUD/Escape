/**
 * @version		$Id: main.c 53 2008-11-15 13:59:04Z nasmussen $
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

#endif /* PROC_H_ */
