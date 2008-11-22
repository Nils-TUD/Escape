/**
 * @version		$Id: debug.h 53 2008-11-15 13:59:04Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef IO_H_
#define IO_H_

#include "common.h"

/* IO flags */
#define IO_READ		1
#define IO_WRITE	2

/**
 * Opens the given path with given mode and returns the associated file-descriptor
 *
 * @param path the path to open
 * @param mode the mode
 * @return the file-descriptor; negative if error
 */
s32 open(cstring path,u8 mode);

/**
 * Closes the given file-descriptor
 *
 * @param fd the file-descriptor
 */
void close(s32 fd);

#endif /* IO_H_ */
