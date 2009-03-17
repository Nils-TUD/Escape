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
 * Resolves the given path to the inode
 *
 * @param e the ext2-handle
 * @param path the path
 * @return the inode or NULL
 */
sInode *ext2_resolvePath(sExt2 *e,string path);

#endif /* PATH_H_ */
