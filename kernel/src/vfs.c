/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/vfs.h"
#include "../h/vfsnode.h"
#include "../h/proc.h"
#include "../h/util.h"
#include "../h/kheap.h"
#include "../h/sched.h"
#include "../h/video.h"
#include <string.h>
#include <sllist.h>

/* max number of open files */
#define FILE_COUNT					(PROC_COUNT * 16)
/* the processes node */
#define PROCESSES()					(nodes + 7)
/* the services node */
#define SERVICES()					(nodes + 10)

/* the initial size of the write-cache for service-usage-nodes */
#define VFS_INITIAL_WRITECACHE		128

/* the public VFS-node (passed to the user) */
typedef struct {
	tVFSNodeNo nodeNo;
	u8 name[MAX_NAME_LEN + 1];
} sVFSNodePub;

/* a message (for communicating with services) */
typedef struct {
	u32 length;
} sMessage;

/**
 * Searches for a free file for the given flags and node-number
 *
 * @param flags the flags (read, write)
 * @param nodeNo the node-number to open
 * @return the file-number on success or the negative error-code
 */
static tFile vfs_getFreeFile(u8 flags,tVFSNodeNo nodeNo);

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

/* global file table */
static sGFTEntry globalFileTable[FILE_COUNT];

void vfs_init(void) {
	sVFSNode *root,*sys;
	vfsn_init();

	/*
	 *  /
	 *   file:
	 *   system:
	 *     |-processes
	 *   services:
	 */
	root = vfsn_createDir(NULL,(string)"",vfs_dirReadHandler);
	/* TODO */
	vfsn_createServiceNode(KERNEL_PID,root,(string)"file",0);
	sys = vfsn_createDir(root,(string)"system",vfs_dirReadHandler);
	vfsn_createDir(sys,(string)"processes",vfs_dirReadHandler);
	vfsn_createDir(root,(string)"services",vfs_dirReadHandler);
}

bool vfs_isValidFile(sGFTEntry *f) {
	return f >= globalFileTable && f < globalFileTable + FILE_COUNT;
}

sGFTEntry *vfs_getFile(tFile no) {
	return globalFileTable + no;
}

tFile vfs_inheritFile(tFile file) {
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n = vfsn_getNode(e->nodeNo);
	/* we can't share multipipe-service-usages since each process has his own node */
	if(n->type == T_SERVUSE && !(n->parent->flags & VFS_SINGLEPIPE)) {
		sVFSNode *child;
		tVFSNodeNo nodeNo;
		tFile newFile;
		s32 err = vfsn_createServiceUse(n->parent,&child);
		if(err < 0)
			return -1;

		nodeNo = NADDR_TO_VNNO(child);
		newFile = vfs_openFile(e->flags,nodeNo);
		if(newFile < 0)
			return -1;
		return newFile;
	}
	else {
		/* just increase references */
		e->refCount++;
		return file;
	}
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

tFile vfs_openFile(u8 flags,tVFSNodeNo nodeNo) {
	sGFTEntry *e;

	/* determine free file */
	tFile f = vfs_getFreeFile(flags,nodeNo);
	if(f < 0)
		return f;

	/* count references of virtual nodes */
	if(IS_VIRT(nodeNo)) {
		sVFSNode *n = nodes + VIRT_INDEX(nodeNo);
		n->refCount++;
	}

	/* unused file? */
	e = globalFileTable + f;
	if(e->flags == 0) {
		e->flags = flags;
		e->refCount = 1;
		e->position = 0;
		e->nodeNo = nodeNo;
	}
	else
		e->refCount++;

	return f;
}

static tFile vfs_getFreeFile(u8 flags,tVFSNodeNo nodeNo) {
	tFile i;
	tFile freeSlot = ERR_NO_FREE_FD;
	bool write = flags & VFS_WRITE;
	sGFTEntry *e = &globalFileTable[0];

	ASSERT(flags & (VFS_READ | VFS_WRITE),"flags empty");
	ASSERT(!(flags & ~(VFS_READ | VFS_WRITE)),"flags contains invalid bits");
	ASSERT(VIRT_INDEX(nodeNo) < NODE_COUNT,"nodeNo invalid");
	/* ensure that we don't increment usages of an unused slot */
	ASSERT(flags != 0,"No flags given");

	for(i = 0; i < FILE_COUNT; i++) {
		/* used slot and same node? */
		if(e->flags != 0) {
			if(e->nodeNo == nodeNo) {
				/* TODO we have to check wether writing twice to a file is allowed */
				/* TODO should be for service-messages. other situations? */
				/* writing to the same file is not possible */
				if(write && e->flags & VFS_WRITE) {
					e++;
					continue;
				}
				/*return ERR_FILE_IN_USE;*/

				/* if the flags are different we need a different slot */
				if(e->flags == flags)
					return i;
			}
		}
		/* remember free slot */
		else if(freeSlot == ERR_NO_FREE_FD) {
			freeSlot = i;
			/* if we want to write we need our own file anyway */
			if(write)
				break;
		}

		e++;
	}

	return freeSlot;
}

s32 vfs_readFile(tPid pid,sGFTEntry *e,u8 *buffer,u32 count) {
	s32 readBytes;
	if((e->flags & VFS_READ) == 0)
		return ERR_NO_READ_PERM;

	if(IS_VIRT(e->nodeNo)) {
		tVFSNodeNo i = VIRT_INDEX(e->nodeNo);
		sVFSNode *n = nodes + i;

		/* node not present anymore? */
		if(n->name == NULL)
			return ERR_INVALID_FILE;

		/* NOTE: we have to lock service-usages for reading because if we have a single-pipe-
		 * service it would be possible to steal a message if no locking is made.
		 * P1 reads the header of a message (-> message still present), P2 reads the complete
		 * message (-> message deleted). So P1 missed the data of the message. */

		/* wait until the node is unlocked, if necessary */
		if(n->type == T_SERVUSE && n->data.servuse.locked != pid) {
			while(n->data.servuse.locked != INVALID_PID)
				proc_switch();
		}

		/* use the read-handler */
		readBytes = n->readHandler(n,buffer,e->position,count);

		if(n->type == T_SERVUSE) {
			/* store position in first message */
			if(readBytes <= 0) {
				readBytes = -readBytes;
				e->position = 0;
				/* unlock node */
				n->data.servuse.locked = INVALID_PID;
			}
			else {
				e->position += readBytes;
				/* lock node */
				n->data.servuse.locked = pid;
			}
		}
		else {
			e->position += readBytes;
		}
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

		/* don't change the position for service-usages */
		/* since we don't need it and it would cause problems with the next read-op */
		if(n->type != T_SERVUSE)
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

			/* last usage? */
			if(n->name != NULL && --(n->refCount) == 0) {
				/* we have to remove the service-usage-node, if it is one */
				if(n->type == T_SERVUSE) {
					/* if there are message for the service we don't want to throw them away */
					/* if there are any in the receivelist (and no references of the node) we
					 * can throw them away because no one will read them anymore
					 * (it means that the client has already closed the file) */
					/* note also that we can assume that the service is still running since we
					 * would have deleted the whole service-node otherwise */
					if(n->data.servuse.sendList == NULL || sll_length(n->data.servuse.sendList) == 0) {
						/* free send and receive list */
						if(n->data.servuse.recvList != NULL) {
							sll_destroy(n->data.servuse.recvList,true);
							n->data.servuse.recvList = NULL;
						}
						if(n->data.servuse.sendList != NULL) {
							sll_destroy(n->data.servuse.sendList,true);
							n->data.servuse.sendList = NULL;
						}

						/* free node */
						if((n->parent->flags & VFS_SINGLEPIPE) == 0)
							kheap_free(n->name);
						vfsn_removeChild(n->parent,n);
					}
				}
				/* free cache, if present */
				else if(n->type != T_SERVICE && n->data.def.cache != NULL) {
					kheap_free(n->data.def.cache);
					n->data.def.cache = NULL;
				}
			}
		}
		/* mark unused */
		e->flags = 0;
	}
}

s32 vfs_createService(tPid pid,cstring name,u8 type) {
	sVFSNode *serv = SERVICES();
	sVFSNode *n = serv->firstChild;
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
		if(n->type == T_SERVICE && n->owner == pid)
			return ERR_PROC_DUP_SERVICE;
		n = n->next;
	}

	/* copy name to kernel-heap */
	hname = (string)kheap_alloc(len + 1);
	if(hname == NULL)
		return ERR_NOT_ENOUGH_MEM;
	strncpy(hname,name,len);

	/* create node */
	n = vfsn_createServiceNode(pid,serv,hname,type);
	if(n != NULL)
		return NADDR_TO_VNNO(n);

	/* failed, so cleanup */
	kheap_free(hname);
	return ERR_NOT_ENOUGH_MEM;
}

s32 vfs_openIntrptMsgNode(sVFSNode *node) {
	sVFSNode *n = NODE_FIRST_CHILD(node);

	/* not not already present? */
	if(n == NULL || n->owner != KERNEL_PID) {
		n = vfsn_createNode(node,(string)SERVICE_CLIENT_KERNEL);
		if(n == NULL)
			return ERR_NOT_ENOUGH_MEM;

		/* init node */
		n->type = T_SERVUSE;
		n->flags = VFS_READ | VFS_WRITE;
		n->readHandler = &vfs_serviceUseReadHandler;
		n->data.servuse.locked = INVALID_PID;
		n->owner = KERNEL_PID;

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
	return vfs_openFile(VFS_READ | VFS_WRITE,NADDR_TO_VNNO(n));
}

void vfs_closeIntrptMsgNode(tFile f) {
	sGFTEntry *e = vfs_getFile(f);
	vfs_closeFile(e);
}

bool vfs_msgAvailableFor(tPid pid) {
	sVFSNode *n = SERVICES();
	sProc *p = proc_getByPid(pid);
	bool isService = false;
	bool isClient = false;
	tFD i;

	/* at first we check wether the process is a service */
	n = NODE_FIRST_CHILD(n);
	while(n != NULL) {
		if(n->owner == pid) {
			isService = true;
			break;
		}
		n = n->next;
	}

	/* p is a service */
	if(n != NULL) {
		tVFSNodeNo client = vfs_getClient(pid,NADDR_TO_VNNO(n));
		if(vfsn_isValidNodeNo(client))
			return true;
	}

	/* now search through the file-descriptors if there is any message */
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != -1) {
			sGFTEntry *e = vfs_getFile(p->fileDescs[i]);
			if(IS_VIRT(e->nodeNo)) {
				n = vfsn_getNode(e->nodeNo);
				/* service-usage and a message in the receive-list? */
				/* we don't want to check that if it is our own service. because it makes no
				 * sense to read from ourself ;) */
				if(n->type == T_SERVUSE && n->parent->owner != pid) {
					isClient = true;
					if(n->data.servuse.recvList != NULL && sll_length(n->data.servuse.recvList) > 0)
						return true;
				}
			}
		}
	}

	/* if we are no client and no service we'll never receive a message */
	/*if(isClient || isService)*/
		return false;
	/*return true;*/
}

s32 vfs_getClient(tPid pid,tVFSNodeNo no) {
	sVFSNode *n,*node = nodes + no;
	if(node->owner != pid || node->type != T_SERVICE)
		return ERR_NOT_OWN_SERVICE;

	/* search for a slot that needs work */
	n = NODE_FIRST_CHILD(node);
	while(n != NULL) {
		/* data available? */
		if(n->data.servuse.sendList != NULL && sll_length(n->data.servuse.sendList) > 0)
			break;
		n = n->next;
	}

	if(n == NULL)
		return ERR_NO_CLIENT_WAITING;

	return NADDR_TO_VNNO(n);
}

s32 vfs_openClient(tPid pid,tVFSNodeNo no) {
	s32 err;
	tVFSNodeNo client = vfs_getClient(pid,no);
	/* error? */
	if(!vfsn_isValidNodeNo(client))
		return client;

	/* open a file for it so that the service can read and write with it */
	return vfs_openFile(VFS_READ | VFS_WRITE,client);
}

s32 vfs_removeService(tPid pid,tVFSNodeNo nodeNo) {
	sVFSNode *serv = SERVICES();
	sVFSNode *m,*t,*n = nodes + nodeNo;

	ASSERT(vfsn_isValidNodeNo(nodeNo),"Invalid node number %d",nodeNo);

	if(n->owner != pid || n->type != T_SERVICE)
		return ERR_NOT_OWN_SERVICE;

	/* remove childs (service-usages) */
	m = NODE_FIRST_CHILD(n);
	while(m != NULL) {
		t = m->next;
		/* free memory */
		if((n->flags & VFS_SINGLEPIPE) == 0)
			kheap_free(m->name);

		/* free send and receive list */
		if(m->data.servuse.recvList != NULL) {
			sll_destroy(m->data.servuse.recvList,true);
			m->data.servuse.recvList = NULL;
		}
		if(m->data.servuse.sendList != NULL) {
			sll_destroy(m->data.servuse.sendList,true);
			m->data.servuse.sendList = NULL;
		}

		/* remove node */
		vfsn_removeChild(n,m);

		/* to next */
		m = t;
	}

	/* free node */
	kheap_free(n->name);
	vfsn_removeChild(serv,n);
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

	n = vfsn_createInfo(proc,name,handler);
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
			/* free node */
			kheap_free(n->name);
			vfsn_removeChild(proc,n);
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

s32 vfs_defReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count,u32 dataSize,
		readCallBack callback) {
	void *mem;

	ASSERT(node != NULL,"node == NULL");
	ASSERT(buffer != NULL,"buffer == NULL");

	/* can we copy it directly? */
	if(offset == 0 && count == dataSize)
		mem = buffer;
	/* don't waste time in this case */
	else if(offset >= dataSize)
		return 0;
	/* ok, use the heap as temporary storage */
	else {
		mem = kheap_alloc(dataSize);
		if(mem == NULL)
			return 0;
	}

	/* copy values to public struct */
	callback(node,mem);

	/* stored on kheap? */
	if((u32)mem != (u32)buffer) {
		/* correct vars */
		if(offset > dataSize)
			offset = dataSize;
		count = MIN(dataSize - offset,count);
		/* copy */
		if(count > 0)
			memcpy(buffer,(u8*)mem + offset,count);
		/* free temp storage */
		kheap_free(mem);
	}

	return count;
}

s32 vfs_serviceUseReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	sSLList *list;
	sProc *p = proc_getRunning();
	sMessage *msg;

	/* services reads from the send-list */
	if(node->parent->owner == p->pid)
		list = node->data.servuse.sendList;
	/* other processes read from the receive-list */
	else
		list = node->data.servuse.recvList;

	/* list not present or empty? */
	if(list == NULL || sll_length(list) == 0)
		return 0;

	/* get first element and copy data to buffer */
	msg = sll_get(list,0);
	offset = MIN(msg->length - 1,offset);
	count = MIN(msg->length - offset,count);
	/* the data is behind the message */
	memcpy(buffer,(u8*)(msg + 1) + offset,count);

	/*vid_printf("\n%d read msg:\n---\n",p->pid);
	dumpBytes(buffer,count);
	vid_printf("\n---\n");*/

	/* free data and remove element from list if the complete message has been read */
	if(offset + count >= msg->length) {
		kheap_free(msg);
		sll_removeIndex(list,0);
		/* negative because we have read the complete msg */
		return -count;
	}

	return count;
}

static s32 vfs_writeHandler(tPid pid,sVFSNode *n,u8 *buffer,u32 offset,u32 count) {
	void *cache;
	void *oldCache;
	u32 newSize;

	/* determine the cache to use */
	if(n->type == T_SERVUSE) {
		sSLList **list;
		sMessage *msg;
		/* services write to the receive-list (which will be read by other processes) */
		/* special-case: pid == KERNEL_PID: the kernel wants to write to a service */
		if(pid != KERNEL_PID && n->parent->owner == pid)
			list = &(n->data.servuse.recvList);
		/* other processes write to the send-list (which will be read by the service) */
		else
			list = &(n->data.servuse.sendList);

		if(*list == NULL)
			*list = sll_create();

		/* create message and copy data to it */
		msg = kheap_alloc(sizeof(sMessage) + count * sizeof(u8));
		if(msg == NULL)
			return ERR_NOT_ENOUGH_MEM;

		msg->length = count;
		memcpy(msg + 1,buffer,count);

		/*vid_printf("\n%d Wrote msg:\n---\n",pid);
		dumpBytes(buffer,count);
		vid_printf("\n---\n");*/

		/* append to list */
		sll_append(*list,msg);

		/* notify the service */
		if(list == &(n->data.servuse.sendList)) {
			if(n->parent->owner != KERNEL_PID)
				sched_setReady(proc_getByPid(n->parent->owner));
		}
		else {
			if(n->parent->flags & VFS_SINGLEPIPE) {
				/* we don't know who uses the service. Therefore we have to unblock all :/ */
				/* TODO is there a better way? */
				sched_unblockAll();
			}
			else {
				/* notify the process that there is a message */
				/* TODO is there a better way than parsing the pid from the node-name? */
				if(n->owner != KERNEL_PID)
					sched_setReady(proc_getByPid(n->owner));
			}
		}
		return count;
	}

	cache = n->data.def.cache;
	oldCache = cache;

	/* need to create cache? */
	if(cache == NULL) {
		newSize = MAX(count,VFS_INITIAL_WRITECACHE);
		/* check for overflow */
		if(newSize > 0xFFFF)
			newSize = 0xFFFF;
		if(newSize < count)
			return ERR_NOT_ENOUGH_MEM;

		n->data.def.cache = kheap_alloc(newSize);
		cache = n->data.def.cache;
		/* reset position */
		n->data.def.pos = 0;
	}
	/* need to increase cache-size? */
	else if(n->data.def.size < offset + count) {
		/* ensure that we allocate enough memory */
		newSize = MAX(offset + count,(u32)n->data.def.size * 2);
		/* check for overflow */
		if(newSize > 0xFFFF)
			newSize = 0xFFFF;
		if(newSize < offset + count)
			return ERR_NOT_ENOUGH_MEM;

		n->data.def.cache = kheap_realloc(cache,newSize);
		cache = n->data.def.cache;
	}

	/* all ok? */
	if(cache != NULL) {
		/* copy the data into the cache */
		memcpy((u8*)cache + offset,buffer,count);
		/* set total size and number of used bytes */
		n->data.def.size = newSize;
		/* we have checked size for overflow. so it is ok here */
		n->data.def.pos = MAX(n->data.def.pos,offset + count);

		return count;
	}

	/* restore cache */
	n->data.def.cache = oldCache;

	/* make the service ready and hope that we choose them :)
	sched_setReady(n->parent->owner);
	proc_switch();*/
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

		ASSERT((u32)byteCount < (u32)0xFFFF,"Overflow of size and pos detected");

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
			sVFSNode *n = vfsn_getNode(e->nodeNo);
			vid_printf("\tfile @ index %d\n",i);
			vid_printf("\t\tread: %d\n",(e->flags & VFS_READ) ? true : false);
			vid_printf("\t\twrite: %d\n",(e->flags & VFS_WRITE) ? true : false);
			vid_printf("\t\tenv: %s\n",IS_VIRT(e->nodeNo) ? "virtual" : "real");
			vid_printf("\t\tnodeNo: %d\n",VIRT_INDEX(e->nodeNo));
			vid_printf("\t\tpos: %d\n",e->position);
			vid_printf("\t\trefCount: %d\n",e->refCount);
			if(n->type == T_SERVUSE)
				vid_printf("\t\tService-Usage of %s @ %s\n",n->name,n->parent->name);
		}
		e++;
	}
}

#endif
