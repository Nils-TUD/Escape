/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PATH_H_
#define PATH_H_

#include <common.h>
#include "ext2.h"

/**
 * Resolves the given path to the inode-number
 *
 * @param e the ext2-handle
 * @param path the path
 * @return the inode-Number or EXT2_BAD_INO
 */
tInodeNo ext2_resolvePath(sExt2 *e,string path);

#endif /* PATH_H_ */
