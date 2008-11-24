#ifndef CPU_H_
#define CPU_H_

/**
 * @return the timestamp-counter value
 */
extern u64 cpu_rdtsc(void);

/**
 * @return the value of the CR0 register
 */
extern u32 cpu_getCR0(void);

/**
 * @return the value of the CR1 register
 */
extern u32 cpu_getCR1(void);

/**
 * @return the value of the CR2 register, that means the linear address that caused a page-fault
 */
extern u32 cpu_getCR2(void);

/**
 * @return the value of the CR3 register, that means the physical address of the page-directory
 */
extern u32 cpu_getCR3(void);

/**
 * @return the value of the CR4 register
 */
extern u32 cpu_getCR4(void);

#endif /*CPU_H_*/
