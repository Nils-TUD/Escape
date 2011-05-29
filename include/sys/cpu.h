/**
 * $Id$
 */

#ifndef CPU_H_
#define CPU_H_

#include <esc/common.h>
#include <sys/printf.h>

#ifdef __i386__
#include <sys/arch/i586/cpu.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/cpu.h>
#endif

/**
 * @return the timestamp-counter value
 */
uint64_t cpu_rdtsc(void);

/**
 * Prints information about the used CPU into the given string-buffer
 *
 * @param buf the string-buffer
 */
void cpu_sprintf(sStringBuffer *buf);

#if DEBUGGING

void cpu_dbg_print(void);

#endif

#endif /* CPU_H_ */
