#ifndef CPU_H_
#define CPU_H_

/**
 * @return the timestamp-counter value
 */
extern u64 cpu_rdtsc(void);

/**
 * @return the value of the CR2 register, that means the linear address that caused a page-fault
 */
extern u32 cpu_getCR2(void);

#endif /*CPU_H_*/
