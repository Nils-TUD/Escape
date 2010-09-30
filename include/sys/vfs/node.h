/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
void vfsn_init(void);

/**
 * Checks whether the given node-number is valid
 *
 * @param nodeNo the number
 * @return true if so
 */
bool vfsn_isValidNodeNo(tInodeNo nodeNo);

/**
 * Checks whether the given node is a driver-node and belongs to the current process
 *
 * @param nodeNo the node-number
 * @return true if so
 */
bool vfsn_isOwnDriverNode(tInodeNo nodeNo);

/**
 * @param node the node
 * @return the node-number of the given node
 */
tInodeNo vfsn_getNodeNo(sVFSNode *node);

/**
 * @param nodeNo the node-number
 * @return the node for given index
 */
sVFSNode *vfsn_getNode(tInodeNo nodeNo);

/**
 * Fetches the first child from given nodes (taking care for links)
 *
 * @param node the node
 * @return the first child
 */
sVFSNode *vfsn_getFirstChild(sVFSNode *node);

/**
 * Determines the path for the given node. Note that static memory will be used for that!
 * So you have to copy the path to another location if you want to keep the path.
 *
 * @param nodeNo the node-number
 * @return the path
 */
char *vfsn_getPath(tInodeNo nodeNo);

/**
 * Retrieves information about the given node
 *
 * @param nodeNo the node-number
 * @param info will be filled
 * @return 0 on success
 */
s32 vfsn_getNodeInfo(tInodeNo nodeNo,sFileInfo *info);

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
s32 vfsn_resolvePath(const char *path,tInodeNo *nodeNo,bool *created,u16 flags);

/**
 * Removes the last '/' from the path, if necessary, and returns a pointer to the last
 * component of the path.
 *
 * @param path the path
 * @param len the length of the path (will be updated)
 * @return pointer to the last component
 */
char *vfsn_basename(char *path,u32 *len);

/**
 * Removes the last component of the path
 *
 * @param path the path
 * @param len the length of the path
 */
void vfsn_dirname(char *path,u32 len);

/**
 * Finds the child-node with name <name>
 *
 * @param node the parent-node
 * @param name the name
 * @param nameLen the length of the name
 * @return the node or NULL
 */
sVFSNode *vfsn_findInDir(sVFSNode *node,const char *name,u32 nameLen);

/**
 * Creates and appends a (incomplete) node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
sVFSNode *vfsn_createNodeAppend(sVFSNode *parent,char *name);

/**
 * Creates a (incomplete) node
 *
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
sVFSNode *vfsn_createNode(char *name);

/**
 * Creates an info-node
 *
 * @param pid the owner
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @param rwHandler the read-handler
 * @param wrHandler the write-handler
 * @param generated wether the content is generated and therefore the size is not available
 * @return the node
 */
sVFSNode *vfsn_createFile(tPid pid,sVFSNode *parent,char *name,fRead rwHandler,fWrite wrHandler,
		bool generated);

/**
 * Creates a directory-node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
sVFSNode *vfsn_createDir(sVFSNode *parent,char *name);

/**
 * Creates a link in directory <node> with name <name> to <target>
 *
 * @param node the directory-node
 * @param name the name
 * @param target the target-node
 * @return the created node on success or NULL
 */
sVFSNode *vfsn_createLink(sVFSNode *node,char *name,sVFSNode *target);

/**
 * Creates a pipe-container
 *
 * @param parent the parent-node
 * @param name the name
 * @return the node
 */
sVFSNode *vfsn_createPipeCon(sVFSNode *parent,char *name);

/**
 * Creates a driver-node
 *
 * @param pid the process-id to use
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @param flags the flags
 * @return the node
 */
sVFSNode *vfsn_createDriverNode(tPid pid,sVFSNode *parent,char *name,u32 flags);

/**
 * Appends the given node as last child to the parent
 *
 * @param parent the parent
 * @param node the child
 */
void vfsn_appendChild(sVFSNode *parent,sVFSNode *node);

/**
 * Removes the given node including all child-nodes from the parent-node and free's all
 * allocated stuff in it depending on the type.
 *
 * @param n the node
 */
void vfsn_removeNode(sVFSNode *n);

/**
 * Appends a usage-node to the given node and stores the pointer to the new node
 * at <child>. Can be used for driver-usage and pipe-usages.
 *
 * @param pid the process for which the usage should be created
 * @param n the node to which the new node should be appended
 * @param child will contain the pointer to the new node, if successfull
 * @return the error-code if negative or 0 if successfull
 */
s32 vfsn_createUse(tPid pid,sVFSNode *n,sVFSNode **child);

#if DEBUGGING

/**
 * Prints the VFS tree
 */
void vfsn_dbg_printTree(void);

/**
 * Prints the given VFS node
 */
void vfsn_dbg_printNode(sVFSNode *node);

#endif

#endif /* VFSNODE_H_ */
