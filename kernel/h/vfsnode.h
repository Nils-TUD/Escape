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

#include "common.h"
#include "vfs.h"
#include <fsinterface.h>

/*
 * dirs: /, /fs, /system, /system/processes, /system/services
 * files: /system/processes/%, /system/services/%
 */
#define NODE_COUNT					(PROC_COUNT * 8 + 4 + 64)

/* max node-name len */
#define MAX_NAME_LEN				59
#define MAX_PATH_LEN				255

/* determines the node-number (for a virtual node) from the given node-address */
#define NADDR_TO_VNNO(naddr)		((((u32)(naddr) - (u32)&nodes[0]) / sizeof(sVFSNode)) | (1 << 30))

/* fetches the first-child from the given node, taking care of links */
#define NODE_FIRST_CHILD(node)		(!MODE_IS_LINK((node)->mode) ? \
	((node)->firstChild) : (((sVFSNode*)((node)->data.def.cache))->firstChild))

/* checks wether the given node-number is a virtual one */
#define IS_VIRT(nodeNo)				(((nodeNo) & (1 << 30)) != 0)
/* makes a virtual node number */
#define MAKE_VIRT(nodeNo)			((nodeNo) | (1 << 30))
/* removes the virtual-flag */
#define VIRT_INDEX(nodeNo)			((nodeNo) & ~(1 << 30))

/**
 * Initializes the nodes
 */
void vfsn_init(void);

/**
 * Checks wether the given node-number is valid
 *
 * @param nodeNo the number
 * @return true if so
 */
bool vfsn_isValidNodeNo(tVFSNodeNo nodeNo);

/**
 * Checks wether the given node is a service-node and belongs to the current process
 *
 * @param nodeNo the node-number
 * @return true if so
 */
bool vfsn_isOwnServiceNode(tVFSNodeNo nodeNo);

/**
 * @param nodeNo the node-number
 * @return the node for given index
 */
sVFSNode *vfsn_getNode(tVFSNodeNo nodeNo);

/**
 * Determines the path for the given node. Note that static memory will be used for that!
 * So you have to copy the path to another location if you want to keep the path.
 *
 * @param nodeNo the node-number
 * @return the path
 */
char *vfsn_getPath(tVFSNodeNo nodeNo);

/**
 * Retrieves information about the given node
 *
 * @param nodeNo the node-number
 * @param info will be filled
 * @return 0 on success
 */
s32 vfsn_getNodeInfo(tVFSNodeNo nodeNo,sFileInfo *info);

/**
 * Resolves the given path to a VFS-node
 *
 * @param path the path to resolve
 * @param nodeNo the node-number for that path (will be set)
 * @param createNode wether pipe- or service-usage-nodes should be created, if necessary.
 * 	If false an access to pipes and services will be an error.
 * @return 0 if successfull or the error-code
 */
s32 vfsn_resolvePath(const char *path,tVFSNodeNo *nodeNo,bool createNode);

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
 * Creates a directory-node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
sVFSNode *vfsn_createDir(sVFSNode *parent,char *name);

/**
 * Creates a pipe-container
 *
 * @param parent the parent-node
 * @param name the name
 * @return the node
 */
sVFSNode *vfsn_createPipeCon(sVFSNode *parent,char *name);

/**
 * Creates an info-node
 *
 * @param tid the owner
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @param handler the read-handler
 * @return the node
 */
sVFSNode *vfsn_createInfo(tTid tid,sVFSNode *parent,char *name,fRead handler);

/**
 * Creates a service-node
 *
 * @param tid the thread-id to use
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @param type single-pipe or multi-pipe
 * @return the node
 */
sVFSNode *vfsn_createServiceNode(tTid tid,sVFSNode *parent,char *name,u32 type);

/**
 * Creates a service-use-node
 *
 * @param tid the thread-id to use
 * @param parent the parent node
 * @param name the name
 * @param handler the read-handler
 * @return the node or NULL
 */
sVFSNode *vfsn_createServiceUseNode(tTid tid,sVFSNode *parent,char *name,fRead handler);

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
 * Appends a service-usage-node to the given node and stores the pointer to the new node
 * at <child>.
 *
 * @param tid the thread for which the usage should be created
 * @param n the node to which the new node should be appended
 * @param child will contain the pointer to the new node, if successfull
 * @return the error-code if negative or 0 if successfull
 */
s32 vfsn_createServiceUse(tTid tid,sVFSNode *n,sVFSNode **child);

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


/* all nodes */
extern sVFSNode nodes[];

#endif /* VFSNODE_H_ */
