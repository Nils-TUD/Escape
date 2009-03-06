/**
 * @version		$Id: vfs.h 87 2008-11-24 14:32:19Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef VFS_H_
#define VFS_H_

#include "../h/common.h"
#include "../h/proc.h"

/* the possible node-types */
typedef enum {T_DIR,T_LINK,T_INFO,T_SERVICE,T_SERVUSE} eNodeType;

/* vfs-node and GFT flags */
enum {VFS_NOACCESS = 0,VFS_READ = 1,VFS_WRITE = 2};

/* a node in our virtual file system */
typedef struct sVFSNode sVFSNode;
/* the function for read-requests on info-nodes */
typedef s32 (*fRead)(sVFSNode *node,u8 *buffer,u32 offset,u32 count);

struct sVFSNode {
	string name;
	u8 type;
	u8 flags;
	u8 refCount;	/* number of open files for this node */
	sVFSNode *parent;
	sVFSNode *prev;
	sVFSNode *next;
	sVFSNode *firstChild;
	sVFSNode *lastChild;
	fRead readHandler;
	union {
		/* for services */
		struct {
			sProc *proc;
		} service;
		/* for service-usages */
		struct {
			u16 readPos;
			/* send channel */
			u16 sendChanSize;	/* size of the buffer */
			u16 sendChanPos;	/* currently used size */
			void *sendChan;
			/* receive channel */
			u16 recvChanSize;	/* size of the buffer */
			u16 recvChanPos;	/* currently used size */
			void *recvChan;
		} servuse;
		/* for all other nodes-types */
		struct {
			u16 size;			/* size of the buffer */
			u16 pos;			/* currently used size */
			void *cache;
		} def;
	} data;
};

/* an entry in the global file table */
typedef struct {
	/* read OR write; flags = 0 => entry unused */
	u8 flags;
	/* number of references */
	u16 refCount;
	/* current position in file */
	u32 position;
	/* node-number; if MSB = 1 => virtual, otherwise real (fs) */
	tVFSNodeNo nodeNo;
} sGFTEntry;

/**
 * Initializes the virtual file system
 */
void vfs_init(void);

/**
 * Checks wether the given node-number is valid
 *
 * @param nodeNo the number
 * @return true if so
 */
bool vfs_isValidNodeNo(tVFSNodeNo nodeNo);

/**
 * Checks if the given file is valid
 *
 * @param f the file
 * @return true if so
 */
bool vfs_isValidFile(sGFTEntry *f);

/**
 * Checks wether the given node is a service-node and belongs to the current process
 *
 * @param node the node
 * @return true if so
 */
bool vfs_isOwnServiceNode(sVFSNode *node);

/**
 * @param nodeNo the node-number
 * @return the node for given index
 */
sVFSNode *vfs_getNode(tVFSNodeNo nodeNo);

/**
 * @param no the file-number
 * @return the entry in the global-file-table
 */
sGFTEntry *vfs_getFile(tFile no);

/**
 * Determines the path for the given node. Note that static memory will be used for that!
 * So you have to copy the path to another location if you want to keep the path.
 *
 * @param node the node
 * @return the path
 */
string vfs_getPath(sVFSNode *node);

/**
 * Resolves the given path to a VFS-node
 *
 * @param path the path to resolve
 * @param nodeNo the node-number for that path (will be set)
 * @return 0 if successfull or the error-code
 */
s32 vfs_resolvePath(cstring path,tVFSNodeNo *nodeNo);

/**
 * Determines the GFT-File for the given file-descriptor
 *
 * @param fd the file-descriptor
 * @return the GFT-File (sGFTEntry*) or a negative error-code
 */
s32 vfs_fdToFile(tFD fd);

/**
 * Opens the file with given number and given flags. That means it walks through the global
 * file table and searches for a free entry or an entry for that file.
 * Note that multiple processes may read from the same file simultaneously but NOT write!
 *
 * @param flags wether it is a virtual or real file and wether you want to read or write
 * @param nodeNo the node-number (in the virtual or real environment)
 * @param fd will be set to the file-descriptor if successfull
 * @return 0 if successfull or < 0 (ERR_FILE_IN_USE, ERR_NO_FREE_FD)
 */
s32 vfs_openFile(u8 flags,tVFSNodeNo nodeNo,tFD *fd);

/**
 * Reads max. count bytes from the given file into the given buffer and returns the number
 * of read bytes.
 *
 * @param e the file
 * @param buffer the buffer to write to
 * @param count the max. number of bytes to read
 * @return the number of bytes read
 */
s32 vfs_readFile(sGFTEntry *e,u8 *buffer,u32 count);

/**
 * Writes count bytes from the given buffer into the given file and returns the number of written
 * bytes.
 *
 * @param pid will be used to check wether the service writes or a service-user
 * @param e the file
 * @param buffer the buffer to read from
 * @param count the number of bytes to write
 * @return the number of bytes written
 */
s32 vfs_writeFile(tPid pid,sGFTEntry *e,u8 *buffer,u32 count);

/**
 * Closes the given file. That means it calls proc_closeFile() and decrements the reference-count
 * in the global file table. If there are no references anymore it releases the slot.
 *
 * @param e the file
 */
void vfs_closeFile(sGFTEntry *e);

/**
 * Creates a service-node for the given process and given name
 *
 * @param pid the process-id
 * @param name the service-name
 * @return 0 if ok, negative if an error occurred
 */
s32 vfs_createService(tPid pid,cstring name);

/**
 * Opens a file for the interrupt-message-sending. Creates a node for it, if not already done
 *
 * @param node the service-node
 * @return the file-number or a negative error-code
 */
s32 vfs_openIntrptMsgNode(sVFSNode *node);

/**
 * Closes the given file for interrupt-message-sending. Removes the node, if no longer used
 *
 * @param f the file
 */
void vfs_closeIntrptMsgNode(tFile f);

/**
 * For services: Waits until a clients wants to be served and returns a file-descriptor
 * for it.
 *
 * @param no the node-number
 * @return the error-code (negative) or the file-descriptor to use
 */
s32 vfs_waitForClient(tVFSNodeNo no);

/**
 * Removes the service with given node-number
 *
 * @param nodeNo the node-number of the service
 */
s32 vfs_removeService(tVFSNodeNo nodeNo);

/**
 * Creates a process-node with given pid and handler-function
 *
 * @param pid the process-id
 * @param handler the read-handler
 * @return true if successfull
 */
bool vfs_createProcess(tPid pid,fRead handler);

/**
 * Removes the node for the process with given id from the VFS
 *
 * @param pid the process-id
 */
void vfs_removeProcess(tPid pid);

#if DEBUGGING

/**
 * Prints all used entries in the global file table
 */
void vfs_dbg_printGFT(void);

/**
 * Prints the VFS tree
 */
void vfs_dbg_printTree(void);

/**
 * Prints the given VFS node
 */
void vfs_dbg_printNode(sVFSNode *node);

/**
 * @return the number of entries in the global file table
 */
u32 vfs_dbg_getGFTEntryCount(void);

#endif

#endif /* VFS_H_ */
