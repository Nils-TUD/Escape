/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/vfs.h"
#include "../h/proc.h"
#include "../h/video.h"
#include "../h/util.h"
#include "../h/kheap.h"
#include "../h/sched.h"
#include <string.h>

/*
 * dirs: /, /fs, /system, /system/processes, /system/services
 * files: /system/processes/%, /system/services/%
 */
#define NODE_COUNT	(PROC_COUNT * 8 + 4 + 64)
/* max number of open files */
#define FILE_COUNT	(PROC_COUNT * 16)
/* the processes node */
#define PROCESSES()	(nodes + 7)
/* the services node */
#define SERVICES() (nodes + 10)

/* max node-name len */
#define MAX_NAME_LEN 59
#define MAX_PATH_LEN (MAX_NAME_LEN * 6)

/* the initial size of the write-cache for service-usage-nodes */
#define VFS_INITIAL_WRITECACHE 128

/* determines the node-number (for a virtual node) from the given node-address */
#define NADDR_TO_VNNO(naddr) ((((u32)(naddr) - (u32)&nodes[0]) / sizeof(sVFSNode)) | (1 << 30))

/* fetches the first-child from the given node, taking care of links */
#define NODE_FIRST_CHILD(node) (((node)->type != T_LINK) ? \
		((node)->firstChild) : (((sVFSNode*)((node)->data.info.cache))->firstChild))

/* checks wether the given node-number is a virtual one */
#define IS_VIRT(nodeNo) (((nodeNo) & (1 << 30)) != 0)
/* makes a virtual node number */
#define MAKE_VIRT(nodeNo) ((nodeNo) | (1 << 30))
/* removes the virtual-flag */
#define VIRT_INDEX(nodeNo) ((nodeNo) & ~(1 << 30))

/* the public VFS-node (passed to the user) */
typedef struct {
	tVFSNodeNo nodeNo;
	u8 name[MAX_NAME_LEN + 1];
} sVFSNodePub;

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
 * Requests a new node and returns the pointer to it. Panics if there are no free nodes anymore.
 *
 * @return the pointer to the node
 */
static sVFSNode *vfs_requestNode(void);

/**
 * Releases the given node
 *
 * @param node the node
 */
static void vfs_releaseNode(sVFSNode *node);

/**
 * Appends the given node as last child to the parent
 *
 * @param parent the parent
 * @param node the child
 */
static void vfs_appendChild(sVFSNode *parent,sVFSNode *node);

/**
 * Removes the given child from the given parent
 *
 * @param parent the parent
 * @param node the child
 */
static void vfs_removeChild(sVFSNode *parent,sVFSNode *node);

/**
 * The recursive function to print the VFS-tree
 *
 * @param level the current recursion level
 * @param parent the parent node
 */
static void vfs_doPrintTree(u32 level,sVFSNode *parent);

/**
 * The read-handler for directories
 *
 * @param node the VFS node
 * @param buffer the buffer where to copy the info to
 * @param offset the offset where to start
 * @param count the number of bytes
 * @return the number of read bytes
 */
static s32 vfs_dirReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The read-handler for service-usages
 *
 * @param node the VFS node
 * @param buffer the buffer where to copy the info to
 * @param offset the offset where to start
 * @param count the number of bytes
 * @return the number of read bytes
 */
static s32 vfs_serviceUseReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * Creates a (incomplete) node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
static sVFSNode *vfs_createNode(sVFSNode *parent,string name);

/**
 * Creates a directory-node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
static sVFSNode *vfs_createDir(sVFSNode *parent,string name);

/**
 * Creates an info-node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @param handler the read-handler
 * @return the node
 */
static sVFSNode *vfs_createInfo(sVFSNode *parent,string name,fRead handler);

/**
 * Creates a service-node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
static sVFSNode *vfs_createServiceNode(sVFSNode *parent,string name);

/**
 * Appends a service-usage-node to the given node and stores the pointer to the new node
 * at <child>.
 *
 * @param n the node to which the new node should be appended
 * @param child will contain the pointer to the new node, if successfull
 * @return the error-code if negative or 0 if successfull
 */
static s32 vfs_createServiceUse(sVFSNode *n,sVFSNode **child);

/**
 * Creates a service-use-node
 *
 * @param parent the parent node
 * @param name the name
 * @return the node or NULL
 */
static sVFSNode *vfs_createServiceUseNode(sVFSNode *parent,string name);

/* all nodes */
static sVFSNode nodes[NODE_COUNT];
/* a pointer to the first free node (which points to the next and so on) */
static sVFSNode *freeList;

/* global file table */
static sGFTEntry globalFileTable[FILE_COUNT];

void vfs_init(void) {
	tVFSNodeNo i;
	sVFSNode *root,*sys,*node = &nodes[0];
	freeList = node;
	for(i = 0; i < NODE_COUNT - 1; i++) {
		node->next = node + 1;
		node++;
	}
	node->next = NULL;

	/*
	 *  /
	 *   |-fs
	 *   \-system
	 *     |-processes
	 *     \-services
	 */
	root = vfs_createDir(NULL,(string)"");
	node = vfs_createServiceNode(root,(string)"fs");
	sys = vfs_createDir(root,(string)"system");
	node = vfs_createDir(sys,(string)"processes");
	node = vfs_createDir(sys,(string)"services");
}

bool vfs_isValidNodeNo(tVFSNodeNo nodeNo) {
	if(IS_VIRT(nodeNo))
		return VIRT_INDEX(nodeNo) < NODE_COUNT;
	return nodeNo < NODE_COUNT;
}

sVFSNode *vfs_getNode(tVFSNodeNo nodeNo) {
	return nodes + nodeNo;
}

string vfs_getPath(sVFSNode *node) {
	static s8 path[MAX_PATH_LEN];
	u32 nlen,len = 0;
	sVFSNode *n = node;
	ASSERT(node != NULL,"node = NULL");

	/* '/' is a special case */
	if(n->parent == NULL) {
		*path = '/';
		len = 1;
	}
	else {
		u32 total = 0;
		while(n->parent != NULL) {
			/* name + slash */
			total += strlen(n->name) + 1;
			n = n->parent;
		}

		/* not nice, but ensures that we don't overwrite something :) */
		if(total > MAX_PATH_LEN)
			total = MAX_PATH_LEN;

		n = node;
		len = total;
		while(n->parent != NULL) {
			nlen = strlen(n->name) + 1;
			/* move the current string forward */
			/* insert the new element */
			*(path + total - nlen) = '/';
			memcpy(path + total + 1 - nlen,n->name,nlen - 1);
			total -= nlen;
			n = n->parent;
		}
	}

	/* terminate */
	*(path + len) = '\0';

	return (string)path;
}

s32 vfs_resolvePath(cstring path,tVFSNodeNo *nodeNo) {
	sVFSNode *n;
	s32 pos;
	/* select start */
	if(*path == '/') {
		n = &nodes[0];
		/* skip slashes (we have at least 1) */
		do {
			path++;
		}
		while(*path == '/');
	}
	else
		panic("TODO: use current path");

	/* root/current node requested? */
	if(!*path) {
		*nodeNo = NADDR_TO_VNNO(n);
		return 0;
	}

	pos = strchri(path,'/');
	n = NODE_FIRST_CHILD(n);
	while(n != NULL) {
		if(strncmp(n->name,path,pos) == 0) {
			path += pos;
			/* finished? */
			if(!*path)
				break;

			/* skip slashes */
			while(*path == '/') {
				path++;
			}
			/* "/" at the end is optional */
			if(!*path)
				break;

			/* TODO ?? */
			if(n->type == T_SERVICE)
				break;

			/* move to childs of this node */
			pos = strchri(path,'/');
			n = NODE_FIRST_CHILD(n);
			continue;
		}
		n = n->next;
	}

	if(n == NULL)
		return ERR_VFS_NODE_NOT_FOUND;

	/* handle service */
	if(n->type == T_SERVICE) {
		sVFSNode *child;
		s32 err = vfs_createServiceUse(n,&child);
		if(err < 0)
			return err;

		/* set new node as resolved one */
		*nodeNo = NADDR_TO_VNNO(child);
		return 0;
	}

	/* resolve link */
	if(n->type == T_LINK)
		n = (sVFSNode*)n->data.info.cache;

	/* virtual node */
	*nodeNo = NADDR_TO_VNNO(n);
	return 0;
}

s32 vfs_openFile(u8 flags,tVFSNodeNo nodeNo,tFD *fd) {
	tFile i;
	s32 res;
	s32 freeSlot = ERR_NO_FREE_FD;
	bool write = flags & VFS_WRITE;
	sGFTEntry *e = &globalFileTable[0];
	sVFSNode *n = vfs_getNode(nodeNo);

	ASSERT(flags & (VFS_READ | VFS_WRITE),"flags empty");
	ASSERT(!(flags & ~(VFS_READ | VFS_WRITE)),"flags contains invalid bits");
	ASSERT(VIRT_INDEX(nodeNo) < NODE_COUNT,"nodeNo invalid");
	ASSERT(fd != NULL,"fd == NULL");
	/* ensure that we don't increment usages of an unused slot */
	ASSERT(flags != 0,"No flags given");

	for(i = 0; i < FILE_COUNT; i++) {
		/* used slot and same node? */
		if(e->flags != 0) {
			if(e->nodeNo == nodeNo) {
				/* ensure that service-usages will be handled with one file */
				/* otherwise we get problems with removing the temporary node... */
				/* writing to the same file is not possible */
				if(n->type != T_SERVUSE && write && e->flags & VFS_WRITE)
					return ERR_FILE_IN_USE;

				/* if the flags are different we need a different slot */
				if(n->type == T_SERVUSE || e->flags == flags) {
					res = proc_openFile(i);
					if(res < 0)
						return res;
					e->refCount++;
					*fd = res;
					return 0;
				}
			}
		}
		/* remember free slot */
		else if(freeSlot == ERR_NO_FREE_FD)
			freeSlot = i;

		e++;
	}

	/* reserve slot */
	if(freeSlot >= 0) {
		res = proc_openFile(freeSlot);
		if(res < 0)
			return res;

		e = &globalFileTable[freeSlot];
		e->flags = flags;
		e->refCount = 1;
		e->position = 0;
		e->nodeNo = nodeNo;
		*fd = res;
	}

	return freeSlot;
}

s32 vfs_readFile(tFD fd,u8 *buffer,u32 count) {
	s32 readBytes;
	sGFTEntry *e;
	tFile fileNo = proc_fdToFile(fd);
	if(fileNo < 0)
		return fileNo;

	/* invalid file-number? */
	if(fileNo >= FILE_COUNT)
		return ERR_INVALID_FILE;

	/* not in use? */
	e = globalFileTable + fileNo;
	if(e->flags == 0)
		return ERR_INVALID_FILE;

	if((e->flags & VFS_READ) == 0)
		return ERR_NO_READ_PERM;

	if(IS_VIRT(e->nodeNo)) {
		tVFSNodeNo i = VIRT_INDEX(e->nodeNo);
		sVFSNode *n = nodes + i;
		sProc *p = proc_getRunning();

		/* node not present anymore? */
		if(n->name == NULL)
			return ERR_INVALID_FILE;

		/* node locked? */
		if(n->activeMode != VFS_READ || n->activePid != p->pid) {
			/* wait until it is unlocked */
			while(n->activeMode != 0) {
				proc_switch();
			}
		}

		/* lock node, if unlocked */
		if(n->type == T_SERVUSE && n->activeMode == 0) {
			n->activeMode = VFS_READ;
			n->activePid = p->pid;
		}

		/* use the read-handler */
		readBytes = n->data.info.readHandler(n,buffer,e->position,count);
		e->position += readBytes;
	}
	else {
		/* TODO redirect to fs-service! */
		panic("No handler for real files yet");
	}

	return readBytes;
}

s32 vfs_writeFile(tFD fd,u8 *buffer,u32 count) {
	s32 writtenBytes;
	sGFTEntry *e;
	u32 size;
	tFile fileNo = proc_fdToFile(fd);
	if(fileNo < 0)
		return fileNo;

	/* invalid file-number? */
	if(fileNo >= FILE_COUNT)
		return ERR_INVALID_FILE;

	/* not in use? */
	e = globalFileTable + fileNo;
	if(e->flags == 0)
		return ERR_INVALID_FILE;

	if((e->flags & VFS_WRITE) == 0)
		return ERR_NO_WRITE_PERM;

	if(IS_VIRT(e->nodeNo)) {
		tVFSNodeNo i = VIRT_INDEX(e->nodeNo);
		sVFSNode *n = nodes + i;
		sProc *p = proc_getRunning();

		/* node not present anymore? */
		if(n->name == NULL)
			return ERR_INVALID_FILE;

		/* node locked? */
		if(n->activeMode != VFS_WRITE || n->activePid != p->pid) {
			/* wait until it is unlocked */
			while(n->activeMode != 0) {
				proc_switch();
			}
		}

		/* lock node, if unlocked */
		if(n->type == T_SERVUSE && n->activeMode == 0) {
			n->activeMode = VFS_WRITE;
			n->activePid = p->pid;
		}

		/* need to create cache? */
		if(n->data.info.cache == NULL) {
			size = MAX(count,VFS_INITIAL_WRITECACHE);
			n->data.info.cache = kheap_alloc(size);
			if(n->data.info.cache == NULL)
				return ERR_NOT_ENOUGH_MEM;
			n->data.info.size = size;
		}
		/* need to increase cache-size? */
		else if(n->data.info.size < e->position + count) {
			/* ensure that we allocate enough memory */
			size = MAX(e->position + count,(u32)n->data.info.size * 2);
			n->data.info.cache = kheap_realloc(n->data.info.cache,size);
			if(n->data.info.cache == NULL)
				return ERR_NOT_ENOUGH_MEM;
			n->data.info.size = size;
		}

		/* copy the data into the cache */
		memcpy((u8*)n->data.info.cache + e->position,buffer,count);

		writtenBytes = count;
		e->position += count;
	}
	else {
		/* TODO redirect to fs-service! */
		panic("No handler for real files yet");
	}

	return writtenBytes;
}

s32 vfs_sendEOT(tFD fd) {
	sGFTEntry *e;
	tFile fileNo = proc_fdToFile(fd);
	if(fileNo < 0)
		return fileNo;

	/* invalid file-number? */
	if(fileNo >= FILE_COUNT)
		return ERR_INVALID_FILE;

	/* not in use? */
	e = globalFileTable + fileNo;
	if(e->flags == 0)
		return ERR_INVALID_FILE;

	if(IS_VIRT(e->nodeNo)) {
		tVFSNodeNo i = VIRT_INDEX(e->nodeNo);
		sVFSNode *n = nodes + i;
		sProc *p = proc_getRunning();

		/* node not present anymore? */
		if(n->name == NULL)
			return ERR_INVALID_FILE;

		if(n->activeMode != 0 && n->activePid == p->pid) {
			/* free cache, if reading is finished */
			if(n->activeMode == VFS_READ && n->data.info.cache != NULL) {
				kheap_free(n->data.info.cache);
				n->data.info.cache = NULL;
			}

			/* remove service from the blocked-list */
			sched_setReady(n->parent->data.proc);

			n->activeMode = 0;
			n->activePid = 0;
			/* reset position */
			e->position = 0;
			return 0;
		}

		return ERR_SERVICE_NOT_IN_USE;
	}
	else {
		/* TODO redirect to fs-service! */
		panic("No handler for real files yet");
	}

	return 0;
}

void vfs_closeFile(tFD fd) {
	sGFTEntry *e;
	tFile fileNo = proc_closeFile(fd);
	if(fileNo < 0)
		return;

	/* invalid file-number? */
	if(fileNo >= FILE_COUNT)
		return;

	/* not in use? */
	e = globalFileTable + fileNo;
	if(e->flags == 0)
		return;

	/* special stuff for service-uses */
	if(IS_VIRT(e->nodeNo)) {
		tVFSNodeNo i = VIRT_INDEX(e->nodeNo);
		sVFSNode *n = nodes + i;
		/* not still in use? */
		if(n->name != NULL) {
			/* unlock just if we own the lock */
			if(n->type == T_SERVUSE && n->activeMode && n->activePid == proc_getRunning()->pid) {
				/* free cache, if reading is finished */
				if(n->activeMode == VFS_READ && n->data.info.cache != NULL) {
					kheap_free(n->data.info.cache);
					n->data.info.cache = NULL;
				}
				n->activeMode = 0;
				n->activePid = 0;
				/* reset */
				e->position = 0;
			}
		}
	}

	/* decrement references */
	if(--(e->refCount) == 0) {
		/* free cache if there is any */
		if(IS_VIRT(e->nodeNo)) {
			tVFSNodeNo i = VIRT_INDEX(e->nodeNo);
			sVFSNode *n = nodes + i;

			/* not still in use? */
			if(n->name != NULL) {
				/* we have to remove the service-usage-node, if it is one */
				if(n->type == T_SERVUSE) {
					kheap_free(n->name);
					vfs_removeChild(n->parent,n);
				}

				/* free cache, if present */
				if(n->type != T_SERVICE && n->data.info.cache != NULL) {
					kheap_free(n->data.info.cache);
					n->data.info.cache = NULL;
				}
			}
		}
		/* mark unused */
		e->flags = 0;
	}
}

s32 vfs_createService(sProc *p,cstring name) {
	sVFSNode *serv = SERVICES();
	sVFSNode *n = serv->firstChild;
	u32 len;
	string hname;

	ASSERT(p != NULL,"p == NULL");
	ASSERT(name != NULL,"name == NULL");

	/* we don't want to have exotic service-names */
	if((len = strlen(name)) == 0 || !isalnumstr(name))
		return ERR_INV_SERVICE_NAME;

	while(n != NULL) {
		/* entry already existing? */
		if(strcmp(n->name,name) == 0)
			return ERR_SERVICE_EXISTS;
		if(n->type == T_SERVICE && n->data.proc == p)
			return ERR_PROC_DUP_SERVICE;
		n = n->next;
	}

	/* copy name to kernel-heap */
	hname = (string)kheap_alloc(len + 1);
	if(hname == NULL)
		return ERR_NOT_ENOUGH_MEM;
	strncpy(hname,name,len);

	/* create node */
	n = vfs_createServiceNode(serv,hname);
	if(n != NULL) {
		n->data.proc = p;
		return NADDR_TO_VNNO(n);
	}

	/* failed, so cleanup */
	kheap_free(hname);
	return ERR_NOT_ENOUGH_MEM;
}

s32 vfs_waitForClient(tVFSNodeNo no) {
	sVFSNode *n,*node = vfs_getNode(no);
	tFD fd;
	s32 err;
	sProc *p = proc_getRunning();
	if(node->data.proc != p || node->type != T_SERVICE)
		return ERR_NOT_OWN_SERVICE;

	/* search for a slot that needs work */
	do {
		n = NODE_FIRST_CHILD(node);
		while(n != NULL) {
			/* writing not in progress and data available? */
			if(n->activeMode == 0 && n->data.info.cache != NULL)
				break;
			n = n->next;
		}

		/* wait until someone wakes us up */
		if(n == NULL) {
			sched_setBlocked(p);
			proc_switch();
		}
	}
	while(n == NULL);

	/* open a file for it and return the file-descriptor so that the service can read and write
	 * with it */
	err = vfs_openFile(VFS_READ | VFS_WRITE,NADDR_TO_VNNO(n),&fd);
	if(err < 0)
		return err;
	return fd;
}

s32 vfs_removeService(tVFSNodeNo nodeNo) {
	sVFSNode *serv = SERVICES();
	sVFSNode *m,*t,*n = vfs_getNode(nodeNo);
	sProc *p = proc_getRunning();

	ASSERT(vfs_isValidNodeNo(nodeNo),"Invalid node number %d",nodeNo);

	if(n->data.proc != p || n->type != T_SERVICE)
		return ERR_NOT_OWN_SERVICE;

	/* remove childs (service-usages) */
	m = NODE_FIRST_CHILD(n);
	while(m != NULL) {
		t = m->next;
		/* free memory */
		kheap_free(m->name);
		if(m->data.info.cache != NULL) {
			kheap_free(m->data.info.cache);
			m->data.info.cache = NULL;
		}
		/* remove and release node */
		vfs_removeChild(n,m);
		vfs_releaseNode(m);

		/* to next */
		m = t;
	}

	vfs_removeChild(serv,n);
	/* free node */
	kheap_free(n->name);
	vfs_releaseNode(n);
	return 0;
}

bool vfs_createProcess(tPid pid,fRead handler) {
	string name;
	sVFSNode *proc = PROCESSES();
	sVFSNode *n = proc->firstChild;

	/* build name */
	name = (string)kheap_alloc(sizeof(s8) * 12);
	if(name == NULL)
		return false;

	itoa(name,pid);

	/* go to last entry */
	while(n != NULL) {
		/* entry already existing? */
		if(strcmp(n->name,name) == 0) {
			kheap_free(name);
			return false;
		}
		n = n->next;
	}

	n = vfs_createInfo(proc,name,handler);
	if(n != NULL) {
		/* invalidate cache */
		if(proc->data.info.cache != NULL) {
			kheap_free(proc->data.info.cache);
			proc->data.info.cache = NULL;
		}
		return true;
	}

	kheap_free(name);
	return false;
}

void vfs_removeProcess(tPid pid) {
	sVFSNode *proc = PROCESSES();
	s8 name[12];
	sVFSNode *n;
	itoa(name,pid);

	/* TODO maybe we should store the node-id in the process-struct? */
	n = proc->firstChild;
	while(n != NULL) {
		/* found node? */
		if(strcmp(n->name,name) == 0) {
			vfs_removeChild(proc,n);
			/* free node */
			kheap_free(n->name);
			vfs_releaseNode(n);
			break;
		}
		n = n->next;
	}

	/* invalidate cache */
	if(proc->data.info.cache != NULL) {
		kheap_free(proc->data.info.cache);
		proc->data.info.cache = NULL;
	}
}

static sVFSNode *vfs_requestNode(void) {
	sVFSNode *node;
	if(freeList == NULL)
		panic("No free vfs-nodes!");

	node = freeList;
	freeList = freeList->next;
	return node;
}

static void vfs_releaseNode(sVFSNode *node) {
	ASSERT(node != NULL,"node == NULL");
	/* mark unused */
	node->name = NULL;
	node->next = freeList;
	freeList = node;
}

static void vfs_appendChild(sVFSNode *parent,sVFSNode *node) {
	ASSERT(node != NULL,"node == NULL");

	if(parent != NULL) {
		if(parent->firstChild == NULL)
			parent->firstChild = node;
		if(parent->lastChild != NULL)
			parent->lastChild->next = node;
		node->prev = parent->lastChild;
		parent->lastChild = node;
	}
	node->parent = parent;
}

static void vfs_removeChild(sVFSNode *parent,sVFSNode *node) {
	ASSERT(parent != NULL,"parent == NULL");
	ASSERT(node != NULL,"node == NULL");

	if(node->prev != NULL)
		node->prev->next = node->next;
	else
		parent->firstChild = node->next;
	if(node->next != NULL)
		node->next->prev = node->prev;
	else
		parent->lastChild = node->prev;
}

static void vfs_doPrintTree(u32 level,sVFSNode *parent) {
	u32 i;
	sVFSNode *n = NODE_FIRST_CHILD(parent);
	while(n != NULL) {
		for(i = 0;i < level;i++)
			vid_printf(" |");
		vid_printf("- %s\n",n->name);
		/* don't recurse for "." and ".." */
		if(strncmp(n->name,".",1) != 0 && strncmp(n->name,"..",2) != 0)
			vfs_doPrintTree(level + 1,n);
		n = n->next;
	}
}

static s32 vfs_serviceUseReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	u32 byteCount;
	/* cache not present? */
	if(node->data.info.cache == NULL)
		return 0;

	if(offset > node->data.info.size)
		offset = node->data.info.size;
	byteCount = MIN(node->data.info.size - offset,count);
	if(byteCount > 0) {
		/* simply copy the data to the buffer */
		memcpy(buffer,(u8*)node->data.info.cache + offset,byteCount);
	}

	return byteCount;
}

static s32 vfs_dirReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	s32 byteCount;

	ASSERT(node != NULL,"node == NULL");
	ASSERT(buffer != NULL,"buffer == NULL");

	/* not cached yet? */
	if(node->data.info.cache == NULL) {
		/* we need the number of bytes first */
		byteCount = 0;
		sVFSNode *n = NODE_FIRST_CHILD(node);
		while(n != NULL) {
			byteCount += sizeof(sVFSNodePub);
			n = n->next;
		}
		node->data.info.size = byteCount;
		if(byteCount > 0) {
			/* now allocate mem on the heap and copy all data into it */
			u8 *childs = (u8*)kheap_alloc(byteCount);
			if(childs == NULL)
				node->data.info.size = 0;
			else {
				node->data.info.cache = childs;
				n = NODE_FIRST_CHILD(node);
				while(n != NULL) {
					u16 len = strlen(n->name) + 1;
					sVFSNodePub *pub = (sVFSNodePub*)childs;
					pub->nodeNo = NADDR_TO_VNNO(n);
					memcpy(pub->name,n->name,len);
					childs += sizeof(sVFSNodePub);
					n = n->next;
				}
			}
		}
	}

	if(offset > node->data.info.size)
		offset = node->data.info.size;
	byteCount = MIN(node->data.info.size - offset,count);
	if(byteCount > 0) {
		/* simply copy the data to the buffer */
		memcpy(buffer,(u8*)node->data.info.cache + offset,byteCount);
	}

	return byteCount;
}

static sVFSNode *vfs_createNode(sVFSNode *parent,string name) {
	sVFSNode *node;

	ASSERT(name != NULL,"name == NULL");

	if(strlen(name) > MAX_NAME_LEN)
		return NULL;

	node = vfs_requestNode();
	if(node == NULL)
		return NULL;

	/* ensure that all values are initialized properly */
	node->name = name;
	node->next = NULL;
	node->prev = NULL;
	node->firstChild = NULL;
	node->lastChild = NULL;
	node->data.info.cache = NULL;
	node->data.info.size = 0;
	node->activeMode = 0;

	vfs_appendChild(parent,node);
	return node;
}

static sVFSNode *vfs_createDir(sVFSNode *parent,string name) {
	sVFSNode *node = vfs_createNode(parent,name);
	if(node == NULL)
		return NULL;

	sVFSNode *dot = vfs_createNode(node,(string)".");
	if(dot == NULL)
		return NULL;
	sVFSNode *dotdot = vfs_createNode(node,(string)"..");
	if(dotdot == NULL)
		return NULL;

	node->type = T_DIR;
	node->flags = VFS_READ;
	node->data.info.readHandler = &vfs_dirReadHandler;
	dot->type = T_LINK;
	dot->data.info.cache = node;
	dotdot->type = T_LINK;
	dotdot->data.info.cache = parent == NULL ? node : parent;
	return node;
}

static sVFSNode *vfs_createInfo(sVFSNode *parent,string name,fRead handler) {
	sVFSNode *node = vfs_createNode(parent,name);
	if(node == NULL)
		return NULL;

	node->type = T_INFO;
	node->flags = VFS_READ;
	node->data.info.readHandler = handler;
	return node;
}

static sVFSNode *vfs_createServiceNode(sVFSNode *parent,string name) {
	sVFSNode *node = vfs_createNode(parent,name);
	if(node == NULL)
		return NULL;

	/* TODO */
	node->type = T_SERVICE;
	node->flags = VFS_NOACCESS;
	node->data.info.readHandler = NULL;
	return node;
}

static s32 vfs_createServiceUse(sVFSNode *n,sVFSNode **child) {
	string name;
	sVFSNode *m;
	sProc *p = proc_getRunning();

	/* check duplicate usage
	m = NODE_FIRST_CHILD(n);
	while(m != NULL) {
		if(m->data.proc == p)
			return ERR_PROC_DUP_SERVICE_USE;
		m = m->next;
	}*/

	/* 32 bit signed int => min -2^31 => 10 digits + minus sign + null-termination = 12 bytes */
	name = kheap_alloc(12 * sizeof(s8));
	if(name == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* create usage-node */
	itoa(name,p->pid);
	m = vfs_createServiceUseNode(n,name);
	if(m == NULL) {
		kheap_free(name);
		return ERR_NOT_ENOUGH_MEM;
	}

	/*m->data.proc = p;*/
	*child = m;
	return 0;
}

static sVFSNode *vfs_createServiceUseNode(sVFSNode *parent,string name) {
	sVFSNode *node = vfs_createNode(parent,name);
	if(node == NULL)
		return NULL;

	node->type = T_SERVUSE;
	node->flags = VFS_READ | VFS_WRITE;
	node->data.info.readHandler = &vfs_serviceUseReadHandler;
	return node;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

u32 vfs_dbg_getGFTEntryCount(void) {
	u32 i,count = 0;
	for(i = 0; i < FILE_COUNT; i++) {
		if(globalFileTable[i].flags != 0)
			count++;
	}
	return count;
}

void vfs_dbg_printGFT(void) {
	tFile i;
	sGFTEntry *e = globalFileTable;
	vid_printf("Global File Table:\n");
	for(i = 0; i < FILE_COUNT; i++) {
		if(e->flags != 0) {
			vid_printf("\tfile @ index %d\n",i);
			vid_printf("\t\tread: %d\n",(e->flags & VFS_READ) ? true : false);
			vid_printf("\t\twrite: %d\n",(e->flags & VFS_WRITE) ? true : false);
			vid_printf("\t\tenv: %s\n",IS_VIRT(e->nodeNo) ? "virtual" : "real");
			vid_printf("\t\tnodeNo: %d\n",VIRT_INDEX(e->nodeNo));
			vid_printf("\t\tpos: %d\n",e->position);
			vid_printf("\t\trefCount: %d\n",e->refCount);
		}
		e++;
	}
}

void vfs_dbg_printTree(void) {
	vid_printf("VFS:\n");
	vid_printf("/\n");
	vfs_doPrintTree(1,&nodes[0]);
}

void vfs_dbg_printNode(sVFSNode *node) {
	vid_printf("VFSNode @ 0x%x:\n",node);
	if(node) {
		vid_printf("\tname: %s\n",node->name);
		vid_printf("\ttype: %s\n",node->type == T_DIR ? "DIR" : node->type == T_INFO ? "INFO" : "SERVICE");
		vid_printf("\tfirstChild: 0x%x\n",node->firstChild);
		vid_printf("\tlastChild: 0x%x\n",node->lastChild);
		vid_printf("\tnext: 0x%x\n",node->next);
		vid_printf("\tprev: 0x%x\n",node->prev);
		if(node->type == T_SERVICE)
			vid_printf("\tProcess: 0x%x\n",node->data.proc);
		else {
			vid_printf("\treadHandler: 0x%x\n",node->data.info.readHandler);
			vid_printf("\tcache: 0x%x\n",node->data.info.cache);
			vid_printf("\tsize: %d\n",node->data.info.size);
		}
	}
}

#endif
