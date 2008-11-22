/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PROC_H_
#define PROC_H_

#include "common.h"
#include "intrpt.h"

/* max number of processes */
#define PROC_COUNT		1024
#define MAX_FD_COUNT	16

/* the process-state which will be saved for context-switching */
typedef struct {
	u32 esp;
	u32 edi;
	u32 esi;
	u32 ebp;
	u32 eflags;
} tProcSave;

/* the process states */
typedef enum {ST_UNUSED = 0,ST_RUNNING = 1,ST_READY = 2,ST_BLOCKED = 3,ST_ZOMBIE = 4} tProcState;

/* represents a process */
/* TODO move stuff for existing processes to the kernel-stack-page */
typedef struct {
	/* process state. see tProcState */
	u8 state;
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
	tProcSave save;
	/* file descriptors: indices of the global file table */
	s16 fileDescs[MAX_FD_COUNT];
} tProc;

/* the area for proc_changeSize() */
typedef enum {CHG_DATA,CHG_STACK} chgArea;

/**
 * Saves the state of the current process in the given area
 *
 * @param saveArea the area where to save the state
 */
extern u32 proc_save(tProcSave *saveArea);

/**
 * Resumes the given state
 *
 * @param pageDir the physical address of the page-dir
 * @param saveArea the area to load the state from
 */
extern u32 proc_resume(u32 pageDir,tProcSave *saveArea);

/**
 * Initializes the process-management
 */
void proc_init(void);

/**
 * Searches for a free pid and returns it or 0 if there is no free process-slot
 *
 * @return the pid or 0
 */
u16 proc_getFreePid(void);

/**
 * @return the running process
 */
tProc *proc_getRunning(void);

/**
 * @param pid the pid of the process
 * @return the process with given pid
 */
tProc *proc_getByPid(u16 pid);

/**
 * Switches to another process
 */
void proc_switch(void);

/**
 * Opens the given file-descriptor. That means the function searches for a free slot in the
 * fd-array of the process and inserts the fd.
 *
 * @param fd the file-descriptor
 * @return 0 if successfull or ERR_MAX_PROC_FDS if an error occurred
 */
s32 proc_openFile(u32 fd);

/**
 * Closes the given file-descriptor. That means it searches for it in the fd-array of the process
 * and marks the slot as unused
 *
 * @param fd the file-descriptor
 */
void proc_closeFile(u32 fd);

/**
 * Clones the current process into the given one, saves the new process in proc_clone() so that
 * it will start there on proc_resume(). The function returns -1 if there is
 * not enough memory.
 *
 * @param newPid the target-pid
 * @return -1 if an error occurred, 0 for parent, 1 for child
 */
s32 proc_clone(u16 newPid);

/**
 * Commit suicide. Marks the current process as destroyable. After the next context-switch
 * the process will be removed.
 */
void proc_suicide(void);

/**
 * Destroyes the given process. That means the process-slot will be marked as "unused" and the
 * paging-structure will be freed.
 *
 * @param p the process
 */
void proc_destroy(tProc *p);

/**
 * Setups the given interrupt-stack for the current process
 *
 * @param frame the interrupt-stack-frame
 */
void proc_setupIntrptStack(tIntrptStackFrame *frame);

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

#endif /*PROC_H_*/
