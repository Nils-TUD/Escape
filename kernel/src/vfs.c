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
#define PROCESSES()	(nodes + 3)

/* determines the node-number (for a virtual node) from the given node-address */
#define NADDR_TO_VNNO(naddr) ((((u32)(naddr) - (u32)&nodes[0]) / sizeof(sVFSNode)) | (1 << 31))

/* checks wether the given node-number is a virtual one */
#define IS_VIRT(nodeNo) (((nodeNo) & (1 << 31)) != 0)
/* makes a virtual node number */
#define MAKE_VIRT(nodeNo) ((nodeNo) | (1 << 31))
/* removes the virtual-flag */
#define VIRT_INDEX(nodeNo) ((nodeNo) & ~(1 << 31))

/* the public VFS-node (passed to the user) */
typedef struct {
	tVFSNodeNo nodeNo;
	u8 nameLen;
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

/* all nodes */
static sVFSNode nodes[NODE_COUNT];
/* a pointer to the first free node (which points to the next and so on) */
static sVFSNode *freeList;

/* global file table */
static sGFTEntry globalFileTable[FILE_COUNT];

/**
 * Requests a new node and returns the pointer to it. Panics if there are no free nodes anymore.
 *
 * @return the pointer to the node
 */
static sVFSNode *vfs_requestNode(void) {
	sVFSNode *node;
	if(freeList == NULL)
		panic("No free vfs-nodes!");

	node = freeList;
	freeList = freeList->next;
	return node;
}

/**
 * Releases the given node
 *
 * @param node the node
 */
static void vfs_releaseNode(sVFSNode *node) {
	node->next = freeList;
	freeList = node;
}

/**
 * The recursive function to print the VFS-tree
 *
 * @param level the current recursion level
 * @param parent the parent node
 */
static void vfs_doPrintTree(u32 level,sVFSNode *parent) {
	u32 i;
	sVFSNode *n = parent->childs;
	while(n != NULL) {
		for(i = 0;i < level;i++)
			vid_printf(" |");
		vid_printf("- %s\n",n->name);
		vfs_doPrintTree(level + 1,n);
		n = n->next;
	}
}

/**
 * The read-handler for directories
 *
 * @param node the VFS node
 * @param buffer the buffer where to copy the info to
 * @param offset the offset where to start
 * @param count the number of bytes
 * @return the number of read bytes
 */
static s32 vfs_dirReadHandler(sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	s32 byteCount;
	/* not cached yet? */
	if(node->dataCache == NULL) {
		/* we need the number of bytes first */
		byteCount = 0;
		sVFSNode *n = node->childs;
		while(n != NULL) {
			byteCount += sizeof(sVFSNodePub) + strlen(n->name) + 1;
			n = n->next;
		}
		node->dataSize = byteCount;
		if(byteCount > 0) {
			/* now allocate mem on the heap and copy all data into it */
			u8 *childs = (u8*)kheap_alloc(byteCount);
			node->dataCache = childs;
			n = node->childs;
			while(n != NULL) {
				u16 len = strlen(n->name) + 1;
				sVFSNodePub *pub = (sVFSNodePub*)childs;
				pub->nodeNo = NADDR_TO_VNNO(n);
				pub->nameLen = len;
				childs += sizeof(sVFSNodePub);
				memcpy(childs,n->name,len);
				childs += len;
				n = n->next;
			}
		}
	}

	byteCount = MIN(node->dataSize - offset,count);
	if(byteCount > 0) {
		/* simply copy the data to the buffer */
		memcpy(buffer,(u8*)node->dataCache + offset,byteCount);
	}

	return byteCount;
}

/**
 * Creates a (incomplete) node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
static sVFSNode *vfs_createNode(sVFSNode *parent,sVFSNode *prev,string name) {
	sVFSNode *node = vfs_requestNode();
	node->name = name;
	node->next = NULL;
	node->childs = NULL;
	node->dataCache = NULL;

	if(prev != NULL)
		prev->next = node;
	else if(parent != NULL)
		parent->childs = node;
	return node;
}

/**
 * Creates a directory-node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
static sVFSNode *vfs_createDir(sVFSNode *parent,sVFSNode *prev,string name) {
	sVFSNode *node = vfs_createNode(parent,prev,name);
	node->type = T_DIR;
	node->readHandler = &vfs_dirReadHandler;
	return node;
}

/**
 * Creates an info-node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @param handler the read-handler
 * @return the node
 */
static sVFSNode *vfs_createInfo(sVFSNode *parent,sVFSNode *prev,string name,fRead handler) {
	sVFSNode *node = vfs_createNode(parent,prev,name);
	node->type = T_INFO;
	node->readHandler = handler;
	return node;
}

/**
 * Creates a service-node
 *
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @return the node
 */
static sVFSNode *vfs_createService(sVFSNode *parent,sVFSNode *prev,string name) {
	sVFSNode *node = vfs_createNode(parent,prev,name);
	/* TODO */
	node->type = T_SERVICE;
	node->readHandler = NULL;
	return node;
}

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
	root = vfs_createDir(NULL,NULL,(string)"");
	node = vfs_createService(root,NULL,(string)"fs");
	sys = vfs_createDir(root,node,(string)"system");
	node = vfs_createDir(sys,NULL,(string)"processes");
	node = vfs_createDir(sys,node,(string)"services");
}

sVFSNode *vfs_getNode(tVFSNodeNo nodeNo) {
	return nodes + nodeNo;
}

s32 vfs_openFile(u8 flags,tVFSNodeNo nodeNo,tFile *file) {
	tFile i;
	s32 freeSlot = ERR_NO_FREE_FD;
	bool write = flags & GFT_WRITE;
	sGFTEntry *e = &globalFileTable[0];

	/* ensure that we don't increment usages of an unused slot */
	if(flags == 0)
		panic("No flags given");

	for(i = 0; i < FILE_COUNT; i++) {
		/* used slot and same node? */
		if(e->flags != 0 && e->nodeNo == nodeNo) {
			/* writing to the same file is not possible */
			if(write && e->flags & GFT_WRITE)
				return ERR_FILE_IN_USE;

			/* if the flags are different we need a different slot */
			if(e->flags == flags) {
				e->refCount++;
				*file = i;
				return 0;
			}
		}
		/* remember free slot */
		else if(freeSlot == ERR_NO_FREE_FD)
			freeSlot = i;

		e++;
	}

	/* reserve slot */
	if(freeSlot >= 0) {
		e = &globalFileTable[freeSlot];
		e->flags = flags;
		e->refCount = 1;
		e->position = 0;
		e->nodeNo = nodeNo;
	}

	*file = freeSlot;
	return 0;
}

s32 vfs_readFile(tFile fileNo,u8 *buffer,u32 count) {
	s32 readBytes;
	sGFTEntry *e;
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
		readBytes = nodes[i].readHandler(nodes + i,buffer,e->position,count);
		e->position += readBytes;
	}
	else {
		/* TODO redirect to fs-service! */
		panic("No handler for real files yet");
	}

	return readBytes;
}

void vfs_closeFile(tFile fileNo) {
	sGFTEntry *e;
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
			if(nodes[i].dataCache != NULL) {
				kheap_free(nodes[i].dataCache);
				nodes[i].dataCache = NULL;
			}
		}
		/* mark unused */
		e->flags = 0;
	}
}

void vfs_printGFT(void) {
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

void vfs_printTree(void) {
	vid_printf("VFS:\n");
	vid_printf("/\n");
	vfs_doPrintTree(1,&nodes[0]);
}

void vfs_printNode(sVFSNode *node) {
	vid_printf("VFSNode @ 0x%x:\n",node);
	if(node) {
		vid_printf("\tname: %s\n",node->name);
		vid_printf("\ttype: %s\n",node->type == T_DIR ? "DIR" : node->type == T_INFO ? "INFO" : "SERVICE");
		vid_printf("\tchilds: 0x%x\n",node->childs);
		vid_printf("\tnext: 0x%x\n",node->next);
		vid_printf("\treadHandler: 0x%x\n",node->readHandler);
	}
}

string vfs_cleanPath(string path) {
	s32 pos = 0;
	bool root;
	string res,last = NULL;

	/* remove additional leading slashes */
	while(path[pos] == '/')
		pos++;
	res = strcut(path,MAX(pos - 1,0));
	/* skip slash */
	if((root = pos > 0))
		path++;

	while(*path) {
		pos = strchri(path,'/');

		/* "." or ".." ? */
		if(*path == '.') {
			bool valid = true;
			u32 count = 1;
			s8 first = *(path + 1),sec;
			/* ".." ? */
			if(first == '.') {
				sec = *(path + 2);
				if(sec != '/' && sec != '\0')
					valid = false;
				else
					count++;
			}
			else if((path - 1) != res) {
				if(first != '/' && first != '\0')
					valid = false;
				else if(path != res && root) {
					/* remove last slash */
					path--;
					count++;
				}
			}

			/* treat other names than "." and ".." as normal names */
			if(!valid) {
				last = path;
				path += pos;
			}
			else {
				/* remove current component */
				if(first != '.' || last == NULL) {
					path = strcut(path,count);
					last = path;
				}
				/* remove current + last component */
				else {
					path = strcut(last,(path - last) + count);
					last = path;
				}
				/* ensure that we keep just one slash */
				if(path != res)
					path--;
			}
		}
		/* simply go to next */
		else {
			last = path;
			path += pos;
		}

		/* handle next slashes */
		pos = 0;
		while(path[pos] == '/')
			pos++;
		/* remove last slash (but not the first!) */
		if((res != path && !*(path + pos)) || !root)
			path = strcut(path,pos);
		/* walk behind slash */
		else {
			path = strcut(path,MAX(pos - 1,0));
			path++;
		}
	}

	/* ensure that the path is not terminated with a "/" */
	if(res != path && *path == '/')
		*path = '\0';

	return res;
}

s32 vfs_resolvePath(cstring path,tVFSNodeNo *nodeNo) {
	sVFSNode *n;
	s32 pos;
	/* select start */
	if(*path == '/') {
		n = &nodes[0];
		path++;
	}
	else
		panic("TODO: use current path");

	/* root/current node requested? */
	if(!*path) {
		*nodeNo = NADDR_TO_VNNO(n);
		return 0;
	}

	pos = strchri(path,'/');
	n = n->childs;
	while(n != NULL) {
		if(strncmp(n->name,path,pos) == 0) {
			path += pos;
			/* finished? */
			if(!*path)
				break;

			/* skip slash */
			path++;
			/* "/" at the end is optional */
			if(!*path)
				break;

			if(n->type == T_SERVICE) {
				/* TODO */
				panic("Handle service-request!");
			}

			/* move to childs of this node */
			pos = strchri(path,'/');
			n = n->childs;
			continue;
		}
		n = n->next;
	}

	if(n == NULL)
		return ERR_VFS_NODE_NOT_FOUND;

	/* virtual node */
	*nodeNo = NADDR_TO_VNNO(n);
	return 0;
}

void vfs_createProcessNode(string name,fRead handler) {
	sVFSNode *proc = PROCESSES();
	sVFSNode *n = proc->childs,*prev = NULL;
	while(n != NULL) {
		prev = n;
		n = n->next;
	}

	vfs_createInfo(proc,prev,name,handler);
}
