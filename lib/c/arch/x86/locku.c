/**
 * $Id$
 */

#include <esc/common.h>
#include <esc/thread.h>

void locku(tULock *l) {
	__asm__ (
		"mov $1,%%ecx;"				/* ecx=1 to lock it for others */
		"lockuLoop:"
		"	xor	%%eax,%%eax;"		/* clear eax */
		"	lock;"					/* lock next instruction */
		"	cmpxchg %%ecx,(%0);"	/* compare l with eax; if equal exchange ecx with l */
		"	jnz		lockuLoop;"		/* try again if not equal */
		: : "D" (l)
	);
}
