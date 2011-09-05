/*
 * common.h -- common definitions
 */


#ifndef _COMMON_H_
#define _COMMON_H_


#define K		1024		/* Kilo */
#define M		(K * K)		/* Mega */

#define MEMORY_SIZE	(12 * M)		/* size of main memory */
#define ROM_BASE	0x20000000	/* physical ROM base address */
#define ROM_SIZE	(256 * K)	/* ROM size */
#define IO_BASE		0x30000000	/* physical I/O base address */

#define IO_DEV_MASK	0x3FF00000	/* I/O device mask */
#define IO_REG_MASK	0x000FFFFF	/* I/O register mask */
#define IO_GRAPH_MASK	0x003FFFFF	/* I/O graphics mask */

#define TIMER_BASE	0x30000000	/* physical timer base address */
#define DISPLAY_BASE	0x30100000	/* physical display base address */
#define KEYBOARD_BASE	0x30200000	/* physical keyboard base address */
#define TERM_BASE	0x30300000	/* physical terminal base address */
#define MAX_NTERMS	2		/* max number of terminals */
#define DISK_BASE	0x30400000	/* physical disk base address */
#define OUTPUT_BASE	0x3F000000	/* physical output device address */
#define GRAPH_BASE	0x3FC00000	/* physical grahics base address */
					/* extends to end of address space */

#define PAGE_SIZE	(4 * K)		/* size of a page and a page frame */
#define OFFSET_MASK	(PAGE_SIZE - 1)	/* mask for offset within a page */
#define PAGE_MASK	(~OFFSET_MASK)	/* mask for page number */

#define INSTRS_PER_MSEC	50000		/* average execution speed */


typedef enum { false, true } Bool;	/* truth values */


typedef unsigned char Byte;		/* 8 bit quantities */
typedef unsigned short Half;		/* 16 bit quantities */
typedef unsigned int Word;		/* 32 bit quantities */


#endif /* _COMMON_H_ */
