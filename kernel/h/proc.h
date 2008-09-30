/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PROC_H_
#define PROC_H_

#include "common.h"
#include "paging.h"

/*#define TEST_PROC*/

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

/* the area for proc_changeSize() */
typedef enum {CHG_DATA,CHG_STACK} chgArea;

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

/**
 * Checks wether the given segment-sizes are valid
 * 
 * @param textPages the number of text-pages
 * @param dataPages the number of data-pages
 * @param stackPages the number of stack-pages
 * @return true if so
 */
bool proc_segSizesValid(u32 textPages,u32 dataPages,u32 stackPages);

/**
 * Changes the size of either the data-segment of the process or the stack-segment
 * If <change> is positive pages will be added and otherwise removed. Added pages
 * will always be cleared.
 * If there is not enough memory the function returns false.
 * Note that the size of the current process (dataPages / stackPages) will be adjusted!
 * 
 * @param change the number of pages to add or remove
 * @param area the data or stack? (CHG_*)
 * @return true if successfull
 */
bool proc_changeSize(s32 change,chgArea area);

#ifdef TEST_PROC
void test_proc(void);
#endif

#endif /*PROC_H_*/
