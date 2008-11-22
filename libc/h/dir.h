/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef DIR_H_
#define DIR_H_

#include "common.h"
#include "io.h"

/* a directory-entry */
typedef struct {
	tVFSNodeNo nodeNo;
	string name;
} sDir;

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
sDir *readdir(tFD dir);

/**
 * Closes the given directory
 *
 * @param dir the file-descriptor
 */
void closedir(tFD dir);


#endif /* DIR_H_ */
