/**
 * $Id$
 */

#ifndef I586_SYSCALLS_H_
#define I586_SYSCALLS_H_

#include <esc/common.h>

/* some convenience-macros */
#define SYSC_SETERROR(stack,errorCode)	((stack)->ebx = (errorCode))
#define SYSC_ERROR(stack,errorCode)		{ ((stack)->ebx = (errorCode)); return; }
#define SYSC_RET1(stack,val)			((stack)->eax = (val))
#define SYSC_RET2(stack,val)			((stack)->edx = (val))
#define SYSC_NUMBER(stack)				((stack)->eax)
#define SYSC_ARG1(stack)				((stack)->ecx)
#define SYSC_ARG2(stack)				((stack)->edx)
#define SYSC_ARG3(stack)				(*((uint32_t*)(stack)->uesp))
#define SYSC_ARG4(stack)				(*(((uint32_t*)(stack)->uesp) + 1))
#define SYSC_ARG5(stack)				(*(((uint32_t*)(stack)->uesp) + 2))
#define SYSC_ARG6(stack)				(*(((uint32_t*)(stack)->uesp) + 3))
#define SYSC_ARG7(stack)				(*(((uint32_t*)(stack)->uesp) + 4))

#endif /* I586_SYSCALLS_H_ */
