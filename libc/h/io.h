/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef IO_H_
#define IO_H_

#include "common.h"

/* IO flags */
#define IO_READ		1
#define IO_WRITE	2

/* a VFS-node */
typedef struct {
	tVFSNodeNo nodeNo;
	u8 nameLen;
} sVFSNode;

/**
 * Opens the given path with given mode and returns the associated file-descriptor
 *
 * @param path the path to open
 * @param mode the mode
 * @return the file-descriptor; negative if error
 */
s32 open(cstring path,u8 mode);

/**
 * Reads count bytes from the given file-descriptor into the given buffer and returns the
 * actual read number of bytes.
 *
 * @param fd the file-descriptor
 * @param buffer the buffer to fill
 * @param count the number of bytes
 * @return the actual read number of bytes; negative if an error occurred
 */
s32 read(tFD fd,void *buffer,u32 count);

/**
 * Closes the given file-descriptor
 *
 * @param fd the file-descriptor
 */
void close(tFD fd);

#endif /* IO_H_ */
