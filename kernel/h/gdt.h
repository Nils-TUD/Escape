/**
 * @version		$Id: gdt.h 65 2008-11-16 21:51:42Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef GDT_H_
#define GDT_H_

#include "../h/common.h"

/* the size of the io-map (in bits) */
#define IO_MAP_SIZE				0xFFFF

/**
 * Inits the GDT
 */
void gdt_init(void);

/**
 * Removes the io-map from the TSS so that an exception will be raised if a user-process
 * access a port
 */
void tss_removeIOMap(void);

/**
 * Checks wether the io-map is set
 *
 * @return true if so
 */
bool tss_ioMapPresent(void);

/**
 * Sets the given io-map. That means it copies IO_MAP_SIZE / 8 bytes from the given address
 * into the TSS
 *
 * @param ioMap the io-map to set
 */
void tss_setIOMap(u8 *ioMap);

#endif /*GDT_H_*/
