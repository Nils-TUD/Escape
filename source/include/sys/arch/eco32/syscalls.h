/**
 * $Id$
 */

#ifndef ECO32_SYSCALLS_H_
#define ECO32_SYSCALLS_H_

#include <esc/common.h>

/* some convenience-macros */
#define SYSC_SETERROR(stack,errorCode)	((stack)->r[4] = (errorCode))
#define SYSC_ERROR(stack,errorCode)		{ ((stack)->r[4] = (errorCode)); return; }
#define SYSC_RET1(stack,val)			((stack)->r[2] = (val))
#define SYSC_RET2(stack,val)			((stack)->r[2] = (val))
#define SYSC_NUMBER(stack)				((stack)->r[2])
#define SYSC_ARG1(stack)				((stack)->r[4])
#define SYSC_ARG2(stack)				((stack)->r[5])
#define SYSC_ARG3(stack)				((stack)->r[6])
#define SYSC_ARG4(stack)				((stack)->r[7])
#define SYSC_ARG5(stack)				((stack)->r[8])
#define SYSC_ARG6(stack)				((stack)->r[9])
#define SYSC_ARG7(stack)				((stack)->r[10])

#endif /* ECO32_SYSCALLS_H_ */
