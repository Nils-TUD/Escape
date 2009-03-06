/**
 * @version		$Id: vfs.c 106 2009-03-05 10:46:13Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/vfs.h"
#include "../h/proc.h"
#include "../h/util.h"
#include "../h/kheap.h"
#include "../h/sched.h"
#include <video.h>
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
		((node)->firstChild) : (((sVFSNode*)((node)->data.def.cache))->firstChild))

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
 * The write-handler for the VFS
 *
 * @param pid the process to use
 * @param node the VFS node
 * @param buffer the buffer where to read from
 * @param offset the offset where to start
 * @param count the number of bytes
 * @return the number of written bytes
 */
static s32 vfs_writeHandler(tPid pid,sVFSNode *n,u8 *buffer,u32 offset,u32 count);

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
 * Cleans the given pipe. That means all data behind <offset> will be moved to the beginning
 * and <pos> will be reduced by <offset>.
 *
 * @param node the node
 * @param cache the pointer to the cache
 * @param pos the pointer to the number of used bytes
 * @param offset the offset where to start
 */
static void vfs_cleanServicePipe(sVFSNode *node,u8 *cache,u16 *pos,u32 offset);

/**
 * Creates and appends a (incomplete) node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
static sVFSNode *vfs_createNodeAppend(sVFSNode *parent,string name);

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

	vid_printf("sizeof(sVFSNode)=%d bytes\n",sizeof(sVFSNode));
}

bool vfs_isValidNodeNo(tVFSNodeNo nodeNo) {
	if(IS_VIRT(nodeNo))
		return VIRT_INDEX(nodeNo) < NODE_COUNT;
	return nodeNo < NODE_COUNT;
}

bool vfs_isValidFile(sGFTEntry *f) {
	return f >= globalFileTable && f < globalFileTable + FILE_COUNT;
}

bool vfs_isOwnServiceNode(sVFSNode *node) {
	sProc *p = proc_getRunning();
	return node->data.service.proc == p && node->type == T_SERVICE;
}

sVFSNode *vfs_getNode(tVFSNodeNo nodeNo) {
	return nodes + nodeNo;
}

sGFTEntry *vfs_getFile(tFile no) {
	return globalFileTable + no;
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
		n = (sVFSNode*)n->data.def.cache;

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
				/* writing to the same file is not possible */
				if(write && e->flags & VFS_WRITE) {
					e++;
					continue;
				}
				/*return ERR_FILE_IN_USE;*/

				/* if the flags are different we need a different slot */
				if(e->flags == flags) {
					res = proc_openFile(i);
					if(res < 0)
						return res;
					/* count references of virtual nodes */
					if(IS_VIRT(nodeNo)) {
						sVFSNode *n = nodes + VIRT_INDEX(nodeNo);
						n->refCount++;
						if(n->type == T_SERVUSE)
							e->position = n->data.servuse.readPos;
					}
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

		/* count references of virtual nodes */
		if(IS_VIRT(nodeNo)) {
			sVFSNode *n = nodes + VIRT_INDEX(nodeNo);
			n->refCount++;
			if(n->type == T_SERVUSE)
				e->position = n->data.servuse.readPos;
		}

		e = &globalFileTable[freeSlot];
		e->flags = flags;
		e->refCount = 1;
		e->position = 0;
		e->nodeNo = nodeNo;
		*fd = res;
	}

	return freeSlot;
}

s32 vfs_fdToFile(tFD fd) {
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

	return (s32)e;
}

s32 vfs_readFile(sGFTEntry *e,u8 *buffer,u32 count) {
	s32 readBytes;
	if((e->flags & VFS_READ) == 0)
		return ERR_NO_READ_PERM;

	if(IS_VIRT(e->nodeNo)) {
		tVFSNodeNo i = VIRT_INDEX(e->nodeNo);
		sVFSNode *n = nodes + i;

		/* node not present anymore? */
		if(n->name == NULL)
			return ERR_INVALID_FILE;

		/* use the read-handler */
		readBytes = n->readHandler(n,buffer,e->position,count);

		/* service-usages have always a read-position of 0 */
		e->position += readBytes;
	}
	else {
		/* TODO redirect to fs-service! */
		panic("No handler for real files yet");
	}

	return readBytes;
}

s32 vfs_writeFile(tPid pid,sGFTEntry *e,u8 *buffer,u32 count) {
	s32 writtenBytes;
	sVFSNode *n;

	if((e->flags & VFS_WRITE) == 0)
		return ERR_NO_WRITE_PERM;

	if(IS_VIRT(e->nodeNo)) {
		tVFSNodeNo i = VIRT_INDEX(e->nodeNo);
		n = nodes + i;

		/* node not present anymore? */
		if(n->name == NULL)
			return ERR_INVALID_FILE;

		/* write to the node */
		writtenBytes = vfs_writeHandler(pid,n,buffer,e->position,count);
		if(writtenBytes < 0)
			return writtenBytes;

		e->position += writtenBytes;
	}
	else {
		/* TODO redirect to fs-service! */
		panic("No handler for real files yet");
	}

	return writtenBytes;
}

void vfs_closeFile(sGFTEntry *e) {
	/* decrement references */
	if(--(e->refCount) == 0) {
		/* free cache if there is any */
		if(IS_VIRT(e->nodeNo)) {
			tVFSNodeNo i = VIRT_INDEX(e->nodeNo);
			sVFSNode *n = nodes + i;
			sProc *p = proc_getRunning();

			/* last usage? */
			if(n->name != NULL && --(n->refCount) == 0) {
				/* we have to remove the service-usage-node, if it is one */
				if(n->type == T_SERVUSE) {
					kheap_free(n->name);
					vfs_removeChild(n->parent,n);

					/* free send and receive buffer */
					if(n->data.servuse.recvChan != NULL) {
						kheap_free(n->data.servuse.recvChan);
						n->data.servuse.recvChan = NULL;
					}
					if(n->data.servuse.sendChan != NULL) {
						kheap_free(n->data.servuse.sendChan);
						n->data.servuse.sendChan = NULL;
					}
				}
				/* free cache, if present */
				else if(n->type != T_SERVICE && n->data.def.cache != NULL) {
					kheap_free(n->data.def.cache);
					n->data.def.cache = NULL;
				}
			}
			/* service ready? */
			else if(p == n->parent->data.service.proc) {
				/* so clean the pipe so that we start at the beginning next time */
				vfs_cleanServicePipe(n,n->data.servuse.sendChan,
						&(n->data.servuse.sendChanPos),e->position);
			}
		}
		/* mark unused */
		e->flags = 0;
	}
}

s32 vfs_createService(tPid pid,cstring name) {
	sVFSNode *serv = SERVICES();
	sVFSNode *n = serv->firstChild;
	sProc *p = proc_getByPid(pid);
	u32 len;
	string hname;

	ASSERT(name != NULL,"name == NULL");

	/* we don't want to have exotic service-names */
	if((len = strlen(name)) == 0 || !isalnumstr(name))
		return ERR_INV_SERVICE_NAME;

	while(n != NULL) {
		/* entry already existing? */
		if(strcmp(n->name,name) == 0)
			return ERR_SERVICE_EXISTS;
		if(n->type == T_SERVICE && n->data.service.proc == p)
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
		n->data.service.proc = p;
		return NADDR_TO_VNNO(n);
	}

	/* failed, so cleanup */
	kheap_free(hname);
	return ERR_NOT_ENOUGH_MEM;
}

s32 vfs_openIntrptMsgNode(sVFSNode *node) {
	sVFSNode *n = NODE_FIRST_CHILD(node);
	s32 err;
	tFD fd;

	/* not not already present? */
	if(n == NULL || strcmp(n->name,"k") != 0) {
		n = vfs_createNode(node,(string)"k");
		if(n == NULL)
			return ERR_NOT_ENOUGH_MEM;

		/* init node */
		n->type = T_SERVUSE;
		n->flags = VFS_READ | VFS_WRITE;
		n->readHandler = &vfs_serviceUseReadHandler;

		/* insert as first child */
		n->prev = NULL;
		n->next = node->firstChild;
		node->firstChild = n;
		if(node->lastChild == NULL)
			node->lastChild = n;
		n->parent = node;
	}

	/* open the file and return it */
	/* we don't need the file-descriptor here */
	err = vfs_openFile(VFS_READ | VFS_WRITE,NADDR_TO_VNNO(n),&fd);
	return err;
}

void vfs_closeIntrptMsgNode(tFile f) {
	sGFTEntry *e = vfs_getFile(f);
	vfs_closeFile(e);
}

s32 vfs_waitForClient(tVFSNodeNo no) {
	sVFSNode *n,*node = vfs_getNode(no);
	tFD fd;
	s32 err;
	sProc *p = proc_getRunning();
	if(node->data.service.proc != p || node->type != T_SERVICE)
		return ERR_NOT_OWN_SERVICE;

	/* search for a slot that needs work */
	do {
		/*vid_printf("Proc 0x%x waits for client\n===BEFORE===\n",p);
		sched_dbg_print();*/
		n = NODE_FIRST_CHILD(node);
		while(n != NULL) {
			/* writing not in progress and data available? */
			if(n->data.servuse.sendChan != NULL && n->data.servuse.sendChanPos > 0)
				break;
			n = n->next;
		}

		/* wait until someone wakes us up */
		if(n == NULL) {
			sched_setBlocked(p);
			/*vid_printf("===AFTER===\n",p);
			sched_dbg_print();*/
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

	if(n->data.service.proc != p || n->type != T_SERVICE)
		return ERR_NOT_OWN_SERVICE;

	/* remove childs (service-usages) */
	m = NODE_FIRST_CHILD(n);
	while(m != NULL) {
		t = m->next;
		/* free memory */
		kheap_free(m->name);
		if(m->data.def.cache != NULL) {
			kheap_free(m->data.def.cache);
			m->data.def.cache = NULL;
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
		if(proc->data.def.cache != NULL) {
			kheap_free(proc->data.def.cache);
			proc->data.def.cache = NULL;
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
	if(proc->data.def.cache != NULL) {
		kheap_free(proc->data.def.cache);
		proc->data.def.cache = NULL;
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
	u8 *cache,*end;
	u16 size,*pos;
	sProc *p = proc_getRunning();

	/* services reads from the send-channel */
	if(node->parent->data.service.proc == p) {
		cache = (u8*)node->data.servuse.sendChan;
		size = node->data.servuse.sendChanSize;
		pos = &(node->data.servuse.sendChanPos);
	}
	/* other processes read from the receive-channel */
	else {
		cache = (u8*)node->data.servuse.recvChan;
		size = node->data.servuse.recvChanSize;
		pos = &(node->data.servuse.recvChanPos);
	}

	/* cache not present? */
	if(cache == NULL)
		return 0;

	if(offset > *pos)
		offset = *pos;
	byteCount = MIN(*pos - offset,count);
	if(byteCount > 0) {
		/* simply copy the data to the buffer */
		memcpy(buffer,cache + offset,byteCount);

		vfs_cleanServicePipe(node,cache,pos,offset + byteCount);
		node->data.servuse.readPos = offset + byteCount;
	}

	return byteCount;
}

static void vfs_cleanServicePipe(sVFSNode *node,u8 *cache,u16 *pos,u32 offset) {
	u8 *end;
	if(offset >= 16384) {
		/* now remove the just read data */
		end = cache + *pos;
		cache = cache + offset;
		while(cache < end) {
			*(cache - (offset)) = *cache;
			cache++;
		}

		/* reduce the used size */
		*pos -= offset;

		/* now we have to reset the position of all files for this node */
		u32 i,x;
		tVFSNodeNo no = NADDR_TO_VNNO(node);
		tPid pid = (tPid)atoi(node->name);
		/* it is enough to search in the fds of the service and the service-using process */
		sProc *procs[2] = {
			proc_getByPid(pid),
			node->parent->data.service.proc
		};
		sProc *p;
		for(x = 0; x < 2; x++) {
			p = procs[x];
			for(i = 0; i < MAX_FD_COUNT; i++) {
				if(p->fileDescs[i] != -1) {
					sGFTEntry *e = vfs_getFile(p->fileDescs[i]);
					/* TODO this does not work. we don't know wether the user reads or writes.. */
					if(e->flags && e->nodeNo == no)
						e->position = x == 1 ? 0 : *pos;
				}
			}
		}

		node->data.servuse.readPos = 0;
	}
}

static s32 vfs_writeHandler(tPid pid,sVFSNode *n,u8 *buffer,u32 offset,u32 count) {
	void **cachePtr;
	void *cache;
	void *oldCache;
	u16 *size,*pos;
	u32 newSize;

	/* determine the cache to use */
	if(n->type == T_SERVUSE) {
		/* services write to the receive-channel (which will be read by other processes) */
		/* special-case: pid >= PROC_COUNT: the kernel wants to write to a service */
		if(pid < PROC_COUNT && n->parent->data.service.proc->pid == pid) {
			cachePtr = &(n->data.servuse.recvChan);
			size = &(n->data.servuse.recvChanSize);
			pos = &(n->data.servuse.recvChanPos);
		}
		/* other processes write to the send-channel (which will be read by the service) */
		else {
			cachePtr = &(n->data.servuse.sendChan);
			size = &(n->data.servuse.sendChanSize);
			pos = &(n->data.servuse.sendChanPos);
		}
	}
	else {
		/* use the default one */
		cachePtr = &(n->data.def.cache);
		size = &(n->data.def.size);
		pos = &(n->data.def.pos);
	}

	cache = *cachePtr;
	oldCache = cache;

	/* need to create cache? */
	if(cache == NULL) {
		newSize = MAX(count,VFS_INITIAL_WRITECACHE);
		/* check for overflow */
		if(newSize > 0xFFFF)
			newSize = 0xFFFF;
		if(newSize < count)
			return ERR_NOT_ENOUGH_MEM;

		*cachePtr = kheap_alloc(newSize);
		cache = *cachePtr;
		/* reset position */
		*pos = 0;
	}
	/* need to increase cache-size? */
	else if(*size < offset + count) {
		/* ensure that we allocate enough memory */
		newSize = MAX(offset + count,(u32)*size * 2);
		/* check for overflow */
		if(newSize > 0xFFFF)
			newSize = 0xFFFF;
		if(newSize < offset + count)
			return ERR_NOT_ENOUGH_MEM;

		*cachePtr = kheap_realloc(cache,newSize);
		cache = *cachePtr;
	}

	/* all ok? */
	if(cache != NULL) {
		/* copy the data into the cache */
		memcpy((u8*)cache + offset,buffer,count);
		/* set total size and number of used bytes */
		*size = newSize;
		/* we have checked size for overflow. so it is ok here */
		*pos = MAX(*pos,offset + count);

		/* if it is a service-usage, notify the service */
		if(n->type == T_SERVUSE)
			sched_setReady(n->parent->data.service.proc);

		return count;
	}

	/* restore cache-ptr */
	*cachePtr = oldCache;

	/* make the service ready and hope that we choose them :) */
	/* TODO we need a function to change to a specific process... */
	sched_setReady(n->parent->data.service.proc);
	proc_switch();
	return ERR_NOT_ENOUGH_MEM;
}

static s32 vfs_dirReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	s32 byteCount;

	ASSERT(node != NULL,"node == NULL");
	ASSERT(buffer != NULL,"buffer == NULL");

	/* not cached yet? */
	if(node->data.def.cache == NULL) {
		/* we need the number of bytes first */
		byteCount = 0;
		sVFSNode *n = NODE_FIRST_CHILD(node);
		while(n != NULL) {
			byteCount += sizeof(sVFSNodePub);
			n = n->next;
		}

		ASSERT((u32)byteCount > (u32)0xFFFF,"Overflow of size and pos detected");

		node->data.def.size = byteCount;
		node->data.def.pos = byteCount;
		if(byteCount > 0) {
			/* now allocate mem on the heap and copy all data into it */
			u8 *childs = (u8*)kheap_alloc(byteCount);
			if(childs == NULL) {
				node->data.def.size = 0;
				node->data.def.pos = 0;
			}
			else {
				node->data.def.cache = childs;
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

	if(offset > node->data.def.size)
		offset = node->data.def.size;
	byteCount = MIN(node->data.def.size - offset,count);
	if(byteCount > 0) {
		/* simply copy the data to the buffer */
		memcpy(buffer,(u8*)node->data.def.cache + offset,byteCount);
	}

	return byteCount;
}

static sVFSNode *vfs_createNodeAppend(sVFSNode *parent,string name) {
	sVFSNode *node = vfs_createNode(parent,name);
	vfs_appendChild(parent,node);
	return node;
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
	node->refCount = 0;
	node->next = NULL;
	node->prev = NULL;
	node->firstChild = NULL;
	node->lastChild = NULL;
	node->data.def.cache = NULL;
	node->data.def.size = 0;
	node->data.def.pos = 0;
	node->data.servuse.readPos = 0;
	return node;
}

static sVFSNode *vfs_createDir(sVFSNode *parent,string name) {
	sVFSNode *node = vfs_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	sVFSNode *dot = vfs_createNodeAppend(node,(string)".");
	if(dot == NULL)
		return NULL;
	sVFSNode *dotdot = vfs_createNodeAppend(node,(string)"..");
	if(dotdot == NULL)
		return NULL;

	node->type = T_DIR;
	node->flags = VFS_READ;
	node->readHandler = &vfs_dirReadHandler;
	dot->type = T_LINK;
	dot->data.def.cache = node;
	dotdot->type = T_LINK;
	dotdot->data.def.cache = parent == NULL ? node : parent;
	return node;
}

static sVFSNode *vfs_createInfo(sVFSNode *parent,string name,fRead handler) {
	sVFSNode *node = vfs_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->type = T_INFO;
	node->flags = VFS_READ;
	node->readHandler = handler;
	return node;
}

static sVFSNode *vfs_createServiceNode(sVFSNode *parent,string name) {
	sVFSNode *node = vfs_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	/* TODO */
	node->type = T_SERVICE;
	node->flags = VFS_NOACCESS;
	node->readHandler = NULL;
	return node;
}

static s32 vfs_createServiceUse(sVFSNode *n,sVFSNode **child) {
	string name;
	sVFSNode *m;
	sProc *p = proc_getRunning();

	/* TODO we should check for duplicates and if so just reserve a different file for it */

	/* check duplicate usage
	m = NODE_FIRST_CHILD(n);
	while(m != NULL) {
		if(m->data.service.proc == p)
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

	/*m->data.service.proc = p;*/
	*child = m;
	return 0;
}

static sVFSNode *vfs_createServiceUseNode(sVFSNode *parent,string name) {
	sVFSNode *node = vfs_createNodeAppend(parent,name);
	if(node == NULL)
		return NULL;

	node->type = T_SERVUSE;
	node->flags = VFS_READ | VFS_WRITE;
	node->readHandler = &vfs_serviceUseReadHandler;
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
			vid_printf("\tProcess: 0x%x\n",node->data.service.proc);
		else if(node->type == T_SERVUSE) {
			vid_printf("\tSendChan: 0x%x\n",node->data.servuse.sendChan);
			vid_printf("\tSendChanSize: %d\n",node->data.servuse.sendChanSize);
			vid_printf("\tSendChanPos: %d\n",node->data.servuse.sendChanPos);
			vid_printf("\tRecvChan: 0x%x\n",node->data.servuse.recvChan);
			vid_printf("\tRecvChanSize: %d\n",node->data.servuse.recvChanSize);
			vid_printf("\tRecvChanPos: %d\n",node->data.servuse.recvChanPos);
		}
		else {
			vid_printf("\treadHandler: 0x%x\n",node->readHandler);
			vid_printf("\tcache: 0x%x\n",node->data.def.cache);
			vid_printf("\tsize: %d\n",node->data.def.size);
			vid_printf("\tpos: %d\n",node->data.def.pos);
		}
	}
}

#endif
