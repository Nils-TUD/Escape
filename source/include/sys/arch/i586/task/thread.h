/**
 * $Id$
 */

#ifndef I586_THREAD_H_
#define I586_THREAD_H_

#include <esc/common.h>
#include <sys/arch/i586/fpu.h>

/* the thread-state which will be saved for context-switching */
typedef struct {
	uint32_t esp;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t eflags;
	uint32_t ebx;
	/* note that we don't need to save eip because when we're done in thread_resume() we have
	 * our kernel-stack back which causes the ret-instruction to return to the point where
	 * we've called thread_save(). the user-eip is saved on the kernel-stack anyway.. */
	/* note also that this only works because when we call thread_save() in proc_finishClone
	 * we take care not to call a function afterwards (since it would overwrite the return-address
	 * on the stack). When we call it in thread_switch() our return-address gets overwritten, but
	 * it doesn't really matter because it looks like this:
	 * if(!thread_save(...)) {
	 * 		// old thread
	 * 		// call functions ...
	 * 		thread_resume(...);
	 * }
	 * So wether we return to the instruction after the call of thread_save and jump below this
	 * if-statement or wether we return to the instruction after thread_resume() doesn't matter.
	 */
} sThreadRegs;

typedef struct {
	/* FPU-state; initially NULL */
	sFPUState *fpuState;
} sThreadArchAttr;

#endif /* I586_THREAD_H_ */
