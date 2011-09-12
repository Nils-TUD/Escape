/**
 * $Id: config.h 239 2011-08-30 18:28:06Z nasmussen $
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "common.h"

// version
#define VERSION				1					// version of the MMIX architecture
#define SUBVERSION			0					// secondary byte of version number
#define SUBSUBVERSION		2					// further qualification of version

// register
#define LREG_NUM			256					// number of local registers: 256, 512 or 1024
#define GREG_NUM			224					// number of global registers: <= 224

// caches
#define ICACHE_BLOCK_NUM	8					// has to be a power of 2; 0 = no IC
#define ICACHE_BLOCK_SIZE	(sizeof(octa) * 4)	// has to be multiple of octa
#define DCACHE_BLOCK_NUM	16					// has to be a power of 2; 0 = no DC
#define DCACHE_BLOCK_SIZE	(sizeof(octa) * 4)	// has to be multiple of octa

// TCs
#define INSTR_TC_SIZE		16					// 0 = no ITC
#define DATA_TC_SIZE		32					// 0 = no DTC

// misc
#define INSTRS_PER_TICK		10000				// average execution speed (1 tick = 1 millisecond)
#define DEF_START_ADDR		0x8000000000001000	// PC at the beginning

// RAM
#define RAM_START_ADDR		0x0000000000000000
#define RAM_END_ADDR		0x0000FFFEFFFFFFFF
#define RAM_DEF_SIZE		(32 * 1024 * 1024)	// default size of RAM in bytes

// ROM
#define ROM_START_ADDR		0x0000FFFF00000000
#define ROM_SIZE			(1024 * 1024)		// size of ROM in bytes

// disk
#define DISK_START_ADDR		0x0003000000000000
#define DISK_IRQ			51
#define DISK_SECTOR_SIZE	512					// sector size in bytes
#define DISK_DELAY			2					// seek start/settle + rotational delay
#define DISK_SEEK			5					// full disk seek time
#define DISK_STARTUP		1000				// disk startup time (until DISK_READY)
#define DISK_BUFFER			0x80000				// start of disk-buffer
#define DISK_BUF_SIZE		(8 * DISK_SECTOR_SIZE)	// disk buffer size

// timer
#define TIMER_START_ADDR	0x0001000000000000
#define TIMER_IRQ			52
#define TIMER_COUNT			20					// number of available timers

// terminal
#define TERM_START_ADDR		0x0002000000000000
#define TERM_START_IRQ		53
#define TERM_MAX			4					// maximum number of terminals
#define TERM_DEF_COUNT		0					// default number of terminals
#define TERM_RCVR_MSEC		5					// input checking interval
#define TERM_XMTR_MSEC		2					// output speed

// output
#define OUTPUT_START_ADDR	0x0004000000000000

// display
#define DISPLAY_START_ADDR	0x0005000000000000

// keyboard
#define KEYBOARD_START_ADDR	0x0006000000000000
#define KEYBOARD_IRQ		61
#define KEYBOARD_MSEC		5					// input checking interval


/**
 * The address the PC should be set to at the beginning. Has to be tetra-aligned.
 */
octa cfg_getStartAddr(void);
void cfg_setStartAddr(octa addr);

/**
 * The size of the RAM in bytes. Has to be octa-aligned and <= RAM_END_ADDR.
 */
octa cfg_getRAMSize(void);
void cfg_setRAMSize(octa size);

/**
 * Whether "test-mode" should be enabled or not. If so, TRAP 0,0,0 causes the CPU to halt.
 */
bool cfg_isTestMode(void);
void cfg_setTestMode(bool test);

/**
 * Whether "user-mode" should be enabled or not. If so, some special-registers will be set and
 * paging will be configured.
 */
bool cfg_isUserMode(void);
void cfg_setUserMode(bool user);

/**
 * The program-file to load into the RAM at the beginning
 */
const char *cfg_getProgram(void);
void cfg_setProgram(const char *file);

/**
 * The ROM-file to load into the ROM at the beginning
 */
const char *cfg_getROM(void);
void cfg_setROM(const char *rom);

/**
 * The file to use as disk-image
 */
const char *cfg_getDiskImage(void);
void cfg_setDiskImage(const char *file);

/**
 * The file to use as output-file (debugging)
 */
const char *cfg_getOutputFile(void);
void cfg_setOutputFile(const char *file);

/**
 * The number of terminals to use (0..TERM_MAX)
 */
int cfg_getTermCount(void);
void cfg_setTermCount(int count);

/**
 * Use the display-keyboard-device?
 */
bool cfg_useDspKbd(void);
void cfg_setUseDspKbd(bool use);

/**
 * Use backtracing?
 */
bool cfg_useBacktracing(void);
void cfg_setUseBacktracing(bool use);

/**
 * The symbol maps for tracing; cfg_setMaps() will make a copy
 */
const char **cfg_getMaps(void);
void cfg_setMaps(const char **m);

#endif /* CONFIG_H_ */
