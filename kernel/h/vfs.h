/**
 * @version		$Id: vfs.h 87 2008-11-24 14:32:19Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef VFS_H_
#define VFS_H_

#include "../h/common.h"
#include "../h/proc.h"
#include <sllist.h>

/* special service-client names */
#define SERVICE_CLIENT_KERNEL		"k"
#define SERVICE_CLIENT_ALL			"a"

/* the possible node-types */
typedef enum {T_DIR,T_LINK,T_INFO,T_SERVICE,T_SERVUSE} eNodeType;

/* vfs-node and GFT flags */
enum {
	/* no read and write */
	VFS_NOACCESS = 0,
	VFS_READ = 1,
	VFS_WRITE = 2,
	/* for services: not a node for each client-process but one for all */
	VFS_SINGLEPIPE = 4
};

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
			sProc *locked;
			sSLList *sendList;
			sSLList *recvList;
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
 * Checks if the given file is valid
 *
 * @param f the file
 * @return true if so
 */
bool vfs_isValidFile(sGFTEntry *f);

/**
 * @param no the file-number
 * @return the entry in the global-file-table
 */
sGFTEntry *vfs_getFile(tFile no);

/**
 * Determines the GFT-File for the given file-descriptor
 *
 * @param fd the file-descriptor
 * @return the GFT-File (sGFTEntry*) or a negative error-code
 */
s32 vfs_fdToFile(tFD fd);

/**
 * Inherits the given file for the current process
 *
 * @param file the file
 * @return file the file to use (may be the same)
 */
tFile vfs_inheritFile(tFile file);

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
 * @param type single-pipe or multi-pipe
 * @return 0 if ok, negative if an error occurred
 */
s32 vfs_createService(tPid pid,cstring name,u8 type);

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
 * Checks wether there is a message for the given process. That if the process is a service
 * and should serve a client or if the process has got a message from a service.
 *
 * @param p the process
 * @return true if there is a message
 */
bool vfs_msgAvailableFor(sProc *p);

/**
 * For services: Looks wether a client wants to be served and return the node-number
 *
 * @param p the service
 * @param no the node-number
 * @return the error-code or the node-number of the client
 */
s32 vfs_getClient(sProc *p,tVFSNodeNo no);

/**
 * Opens a file for a client of the given service-node
 *
 * @param no the service-node-number
 * @return the error-code (negative) or the file-descriptor to use
 */
s32 vfs_openClient(tVFSNodeNo no);

/**
 * Removes the service with given node-number
 *
 * @param pid the process-id
 * @param nodeNo the node-number of the service
 */
s32 vfs_removeService(tPid pid,tVFSNodeNo nodeNo);

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

/**
 * The read-handler for service-usages
 *
 * @param node the VFS node
 * @param buffer the buffer where to copy the info to
 * @param offset the offset where to start
 * @param count the number of bytes
 * @return the number of read bytes
 */
s32 vfs_serviceUseReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count);

#if DEBUGGING

/**
 * Prints all used entries in the global file table
 */
void vfs_dbg_printGFT(void);

/**
 * @return the number of entries in the global file table
 */
u32 vfs_dbg_getGFTEntryCount(void);

#endif

#endif /* VFS_H_ */
