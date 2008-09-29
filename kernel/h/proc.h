/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PROC_H_
#define PROC_H_

#include "common.h"
#include "paging.h"

/* max number of processes */
#define PROC_COUNT 1024

typedef struct {
	/* process id (2^16 processes should be enough :)) */
	u16 pid;
	/* parent process id */
	u16 parentPid;
	/* the physical address for the page-directory of this process */
	u32 physPDirAddr;
	/* the number of pages per segment */
	u32 textPages;
	u32 dataPages;
	u32 stackPages;
} tProc;

extern tProc procs[PROC_COUNT];

/* the current process (index in procs) */
/* TODO keep that? */
extern u32 pi;

/**
 * Initializes the process-management
 */
void proc_init(void);

/**
 * Clones the current process into the given one. The function returns false if there is
 * not enough memory.
 * 
 * @param p the target-process
 * @return true if successfull
 */
bool proc_clone(tProc *p);

#endif /*PROC_H_*/
