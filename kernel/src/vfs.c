/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/vfs.h"
#include "../h/proc.h"
#include "../h/video.h"
#include "../h/string.h"
#include "../h/util.h"
#include "../h/kheap.h"

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
#define SERVICES() (nodes + 8)

/* max node-name len */
#define MAX_NAME_LEN 59

/* determines the node-number (for a virtual node) from the given node-address */
#define NADDR_TO_VNNO(naddr) ((((u32)(naddr) - (u32)&nodes[0]) / sizeof(sVFSNode)) | (1 << 31))

/* fetches the first-child from the given node, taking care of links */
#define NODE_FIRST_CHILD(node) (((node)->type != T_LINK) ? \
		((node)->firstChild) : (((sVFSNode*)((node)->data.info.cache))->firstChild))

/* checks wether the given node-number is a virtual one */
#define IS_VIRT(nodeNo) (((nodeNo) & (1 << 31)) != 0)
/* makes a virtual node number */
#define MAKE_VIRT(nodeNo) ((nodeNo) | (1 << 31))
/* removes the virtual-flag */
#define VIRT_INDEX(nodeNo) ((nodeNo) & ~(1 << 31))

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
 * Creates a service-queue-node
 *
 * @param parent the parent node
 * @param name the name
 * @return the node or NULL
 */
static sVFSNode *vfs_createServiceQueueNode(sVFSNode *parent,string name);

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

sVFSNode *vfs_getNode(tVFSNodeNo nodeNo) {
	return nodes + nodeNo;
}

s32 vfs_openFile(u8 flags,tVFSNodeNo nodeNo,tFD *fd) {
	tFile i;
	s32 res;
	s32 freeSlot = ERR_NO_FREE_FD;
	bool write = flags & GFT_WRITE;
	sGFTEntry *e = &globalFileTable[0];

	ASSERT(flags & (GFT_READ | GFT_WRITE),"flags empty");
	ASSERT(!(flags & ~(GFT_READ | GFT_WRITE)),"flags contains invalid bits");
	ASSERT(VIRT_INDEX(nodeNo) < NODE_COUNT,"nodeNo invalid");
	ASSERT(fd != NULL,"fd == NULL");

	/* ensure that we don't increment usages of an unused slot */
	if(flags == 0)
		panic("No flags given");

	for(i = 0; i < FILE_COUNT; i++) {
		/* used slot and same node? */
		if(e->flags != 0) {
			if(e->nodeNo == nodeNo) {
				/* writing to the same file is not possible */
				if(write && e->flags & GFT_WRITE)
					return ERR_FILE_IN_USE;

				/* if the flags are different we need a different slot */
				if(e->flags == flags) {
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

	if((e->flags & GFT_READ) == 0)
		return ERR_NO_READ_PERM;

	if(IS_VIRT(e->nodeNo)) {
		tVFSNodeNo i = VIRT_INDEX(e->nodeNo);
		/* use the read-handler */
		readBytes = nodes[i].data.info.readHandler(nodes + i,buffer,e->position,count);
		e->position += readBytes;
	}
	else {
		/* TODO redirect to fs-service! */
		panic("No handler for real files yet");
	}

	return readBytes;
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

	/* decrement references */
	if(--(e->refCount) == 0) {
		/* free cache if there is any */
		if(IS_VIRT(e->nodeNo)) {
			tVFSNodeNo i = VIRT_INDEX(e->nodeNo);
			if(nodes[i].type != T_SERVICE && nodes[i].data.info.cache != NULL) {
				kheap_free(nodes[i].data.info.cache);
				nodes[i].data.info.cache = NULL;
			}
		}
		/* mark unused */
		e->flags = 0;
	}
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

			if(n->type == T_SERVICE) {
				/* TODO */
				panic("Handle service-request!");
			}

			/* move to childs of this node */
			pos = strchri(path,'/');
			n = NODE_FIRST_CHILD(n);
			continue;
		}
		n = n->next;
	}

	if(n == NULL)
		return ERR_VFS_NODE_NOT_FOUND;

	/* resolve link */
	if(n->type == T_LINK)
		n = (sVFSNode*)n->data.info.cache;

	/* virtual node */
	*nodeNo = NADDR_TO_VNNO(n);
	return 0;
}

s32 vfs_enqueueForService(sProc *p,sVFSNode *service) {
	string name = kheap_alloc(12 * sizeof(s8));
	if(name == NULL)
		return ERR_NOT_ENOUGH_MEM;

	itoa(name,p->pid);
	sVFSNode *node = vfs_createServiceQueueNode(service,name);
	if(node == NULL) {
		kheap_free(name);
		return ERR_NOT_ENOUGH_MEM;
	}

	vfs_appendChild(service,node);
	return 0;
}

s32 vfs_createService(sProc *p,cstring name) {
	sVFSNode *serv = SERVICES();
	sVFSNode *n = serv->firstChild;
	u32 len;
	string hname;

	ASSERT(p != NULL,"p == NULL");

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
		return 0;
	}

	/* failed, so cleanup */
	kheap_free(hname);
	return ERR_NOT_ENOUGH_MEM;
}

void vfs_removeService(sProc *p) {
	sVFSNode *serv = SERVICES();
	sVFSNode *n = serv->firstChild,*prev = NULL;

	ASSERT(p != NULL,"p == NULL");

	while(n != NULL) {
		/* process found? */
		if(n->type == T_SERVICE && n->data.proc == p) {
			vfs_removeChild(serv,n);
			/* free node */
			kheap_free(n->name);
			vfs_releaseNode(n);
			break;
		}
		prev = n;
		n = n->next;
	}
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
	itoa(name,pid);

	/* TODO maybe we should store the node-id in the process-struct? */
	sVFSNode *n = proc->firstChild;
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
	if(strlen(name) > MAX_NAME_LEN)
		return NULL;

	node = vfs_requestNode();
	if(node == NULL)
		return NULL;

	node->name = name;
	node->next = NULL;
	node->prev = NULL;
	node->firstChild = NULL;
	node->lastChild = NULL;
	node->data.info.cache = NULL;
	node->data.info.size = 0;

	vfs_appendChild(parent,node);
	return node;
}

static sVFSNode *vfs_createDir(sVFSNode *parent,string name) {
	sVFSNode *node = vfs_createNode(parent,name);
	if(node == NULL)
		return NULL;

	sVFSNode *dot = vfs_createNode(node,".");
	if(dot == NULL)
		return NULL;
	sVFSNode *dotdot = vfs_createNode(node,"..");
	if(dotdot == NULL)
		return NULL;

	node->type = T_DIR;
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
	node->data.info.readHandler = handler;
	return node;
}

static sVFSNode *vfs_createServiceNode(sVFSNode *parent,string name) {
	sVFSNode *node = vfs_createNode(parent,name);
	if(node == NULL)
		return NULL;

	/* TODO */
	node->type = T_SERVICE;
	node->data.info.readHandler = NULL;
	return node;
}

static sVFSNode *vfs_createServiceQueueNode(sVFSNode *parent,string name) {
	sVFSNode *node = vfs_createNode(parent,name);
	if(node == NULL)
		return NULL;

	node->type = T_SERVQUEUE;
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
			vid_printf("\t\tread: %d\n",(e->flags & GFT_READ) ? true : false);
			vid_printf("\t\twrite: %d\n",(e->flags & GFT_WRITE) ? true : false);
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
