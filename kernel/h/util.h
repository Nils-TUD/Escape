/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

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
 * @param fmt the format of the message to display
 */
void panic(string fmt,...);

/**
 * Prints the memory from <addr> to <addr> + <dwordCount>
 *
 * @param addr the staring address
 * @param dwordCount the number of dwords to print
 */
void dumpMem(void *addr,u32 dwordCount);

/**
 * Copies <len> words from <src> to <dest>
 *
 * @param dest the destination-address
 * @param src the source-address
 * @param len the number of words to copy
 */
void *memcpy(void *dest,const void *src,size_t len);

/**
 * Compares <count> words of <str1> and <str2> and returns wether they are equal
 *
 * @param str1 the first data
 * @param str2 the second data
 * @param count the number of words
 * @return -1 if <str1> lt <str2>, 0 if equal and 1 if <str1> gt <str2>
 */
s32 memcmp(const void *str1,const void *str2,size_t count);

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
