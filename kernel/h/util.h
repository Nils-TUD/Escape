#ifndef UTIL_H_
#define UTIL_H_

#include "../h/common.h"

/**
 * Assembler routine to halt the processor
 */
extern void halt(void);

/**
 * Outputs the <val> to the I/O-Port <port>
 * 
 * @param port the port
 * @param val the value
 */
extern void outb(u16 port,u8 val);

/**
 * Reads the value from the I/O-Port <port>
 * 
 * @param port the port
 * @return the value
 */
extern u8 inb(u16 port);

/**
 * PANIC: Displays the given message and halts
 * 
 * @param msg the message to display 
 */
void panic(char *msg);

/**
 * Prints the memory from <addr> to <addr> + <dwordCount>
 * 
 * @param addr the staring address
 * @param dwordCount the number of dwords to print
 */
void dumpMem(void *addr,u32 dwordCount);

/**
 * Sets all dwords in memory beginning at <addr> and ending at <addr> + <count>
 * to <value>.
 * 
 * @param addr the starting address
 * @param value the value to set
 * @param count the number of dwords
 */
void memset(void *addr,u32 value,u32 count);

#endif /*UTIL_H_*/
