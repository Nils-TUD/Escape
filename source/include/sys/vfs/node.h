/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef VFSNODE_H_
#define VFSNODE_H_

#include <sys/common.h>
#include <sys/vfs/vfs.h>
#include <esc/fsinterface.h>

/**
 * Initializes the nodes
 */
void vfs_node_init(void);

/**
 * Checks whether the given node-number is valid
 *
 * @param nodeNo the number
 * @return true if so
 */
bool vfs_node_isValid(inode_t nodeNo);

/**
 * @param node the node
 * @return the node-number of the given node
 */
inode_t vfs_node_getNo(const sVFSNode *node);

/**
 * Requests the node with given number, i.e. locks it.
 *
 * @param nodeNo the node-number
 * @return the node
 */
sVFSNode *vfs_node_request(inode_t nodeNo);

/**
 * Releases the given node, i.e. unlocks it.
 *
 * @param node the node
 */
void vfs_node_release(sVFSNode *node);

/**
 * @param nodeNo the node-number
 * @return the node for given index
 */
sVFSNode *vfs_node_get(inode_t nodeNo);

/**
 * 'Opens' the given directory, i.e. locks it, and returns the first child.
 *
 * @param dir the directory
 * @param locked whether the directory should be locked
 * @return the first child-node
 */
sVFSNode *vfs_node_openDir(sVFSNode *dir,bool locked);

/**
 * 'Closes' the given directory, i.e. releases the lock.
 *
 * @param dir the directory
 * @param locked whether the directory is locked
 */
void vfs_node_closeDir(sVFSNode *dir,bool locked);

/**
 * Determines the path for the given node. Note that static memory will be used for that!
 * So you have to copy the path to another location if you want to keep the path.
 *
 * @param nodeNo the node-number
 * @return the path
 */
char *vfs_node_getPath(inode_t nodeNo);

/**
 * Retrieves information about the given node
 *
 * @param nodeNo the node-number
 * @param info will be filled
 * @return 0 on success
 */
int vfs_node_getInfo(inode_t nodeNo,sFileInfo *info);

/**
 * Changes the mode of the given node
 *
 * @param pid the process-id
 * @param nodeNo the node-number
 * @param mode the new mode
 * @return 0 on success
 */
int vfs_node_chmod(pid_t pid,inode_t nodeNo,mode_t mode);

/**
 * Changes the owner and group of the given node
 *
 * @param pid the process-id
 * @param nodeNo the node-number
 * @param uid the new user
 * @param gid the new group
 * @return 0 on success
 */
int vfs_node_chown(pid_t pid,inode_t nodeNo,uid_t uid,gid_t gid);

/**
 * Resolves the given path to a VFS-node
 *
 * @param path the path to resolve
 * @param nodeNo the node-number for that path (will be set)
 * @param created will be set to true if the (virtual) node didn't exist and has been created (may
 * 	be NULL if you don't care about it)
 * @param flags the flags (VFS_*) with which to resolve the path (create file,...)
 * @return 0 if successfull or the error-code
 */
int vfs_node_resolvePath(const char *path,inode_t *nodeNo,bool *created,uint flags);

/**
 * Removes the last '/' from the path, if necessary, and returns a pointer to the last
 * component of the path.
 *
 * @param path the path
 * @param len the length of the path (will be updated)
 * @return pointer to the last component
 */
char *vfs_node_basename(char *path,size_t *len);

/**
 * Removes the last component of the path
 *
 * @param path the path
 * @param len the length of the path
 */
void vfs_node_dirname(char *path,size_t len);

/**
 * Finds the child-node with name <name>
 *
 * @param nodeNo the parent-node
 * @param name the name
 * @param nameLen the length of the name
 * @return the node or NULL
 */
sVFSNode *vfs_node_findInDir(inode_t nodeNo,const char *name,size_t nameLen);

/**
 * Creates a (incomplete) node without appending it
 *
 * @param pid the process-id
 * @param name the node-name
 * @return the node
 */
sVFSNode *vfs_node_create(pid_t pid,char *name);

/**
 * Appends the given node to <parent>.
 *
 * @param parent the parent-node (locked)
 * @param node the node to append
 */
void vfs_node_append(sVFSNode *parent,sVFSNode *node);

/**
 * Removes the given node including all child-nodes from the parent-node and free's all
 * allocated stuff in it depending on the type.
 *
 * @param n the node
 */
void vfs_node_destroy(sVFSNode *n);

/**
 * Generates a unique id for given pid
 *
 * @param pid the process-id
 * @return the name, allocated on the heap or NULL
 */
char *vfs_node_getId(pid_t pid);

/**
 * Prints the VFS tree
 */
void vfs_node_printTree(void);

/**
 * Prints the given VFS node
 */
void vfs_node_printNode(const sVFSNode *node);

#endif /* VFSNODE_H_ */
