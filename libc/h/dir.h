/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef DIR_H_
#define DIR_H_

#include "common.h"
#include "io.h"

#define MAX_NAME_LEN 255

/* a directory-entry */
typedef struct {
	tVFSNodeNo nodeNo;
	u16 recLen;
	u8 nameLen;
	u8 fileType;
	s8 name[];
} __attribute__((packed)) sDirEntry;

/**
 * Opens the given directory
 *
 * @param path the path to the directory
 * @return the file-descriptor for the directory or a negative error-code
 */
s32 opendir(cstring path);

/**
 * Reads the next directory-entry from the given file-descriptor.
 * Note that the data of the entry might be overwritten by the next call of readdir()!
 *
 * @param dir the file-descriptor
 * @return a pointer to the directory-entry or NULL if the end has been reached
 */
sDirEntry *readdir(tFD dir);

/**
 * Closes the given directory
 *
 * @param dir the file-descriptor
 */
void closedir(tFD dir);


#endif /* DIR_H_ */
