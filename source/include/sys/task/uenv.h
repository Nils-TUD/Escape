/**
 * $Id$
 */

#ifndef UENV_H_
#define UENV_H_

#include <esc/common.h>
#include <sys/intrpt.h>
#include <sys/task/elf.h>

/**
 * Checks whether a signal should be handled. If so, it will be stored for later finishing and a
 * thread-switch is done, if necessary.
 */
void uenv_handleSignal(void);

/**
 * @return true if a signal-handling should be started. if so, uenv_startSignalHandler() should be
 * 	called
 */
bool uenv_hasSignalToStart(void);

/**
 * Delivers the signal to the user-process, i.e. it arranges things so that the signal-handler is
 * called.
 *
 * @param stack the interrupt-stack-frame
 */
void uenv_startSignalHandler(sIntrptStackFrame *stack);

/**
 * Finishes the signal-handling-process
 *
 * @param stack the interrupt-stack-frame
 * @return 0 on success
 */
int uenv_finishSignalHandler(sIntrptStackFrame *stack);

/**
 * Setups the user-stack for given interrupt-stack, when starting the current process
 *
 * @param frame the interrupt-stack-frame
 * @param path the executable-path
 * @param argc the argument-count
 * @param args the arguments on after another, allocated on the heap; may be NULL
 * @param argsSize the total number of bytes for the arguments (just the data)
 * @param info startup-info
 * @param entryPoint the entry-point
 * @return true if successfull
 */
bool uenv_setupProc(sIntrptStackFrame *frame,const char *path,
		int argc,const char *args,size_t argsSize,const sStartupInfo *info,uintptr_t entryPoint);

/**
 * Setups the user-environment for the given interrupt-stack, when starting the current thread
 *
 * @param frame the interrupt-stack-frame
 * @param arg the thread-argument
 * @param tentryPoint the entry-point
 */
bool uenv_setupThread(sIntrptStackFrame *frame,const void *arg,uintptr_t tentryPoint);

#endif /* UENV_H_ */
