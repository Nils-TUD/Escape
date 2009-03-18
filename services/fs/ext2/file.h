/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef FILE_H_
#define FILE_H_

#include <common.h>
#include "ext2.h"

/**
 * Reads <count> bytes at <offset> into <buffer> from the inode with given number
 *
 * @param e the ext2-handle
 * @param inodeNo the inode-number
 * @param buffer the buffer
 * @param offset the offset
 * @param count the number of bytes to read
 * @return the number of read bytes
 */
s32 ext2_readFile(sExt2 *e,tInodeNo inodeNo,u8 *buffer,u32 offset,u32 count);

#endif /* FILE_H_ */
