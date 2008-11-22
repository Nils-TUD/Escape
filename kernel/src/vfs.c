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

/*
 * dirs: /, /fs, /system, /system/processes, /system/services
 * files: /system/processes/%, /system/services/%
 */
#define NODE_COUNT	(PROC_COUNT * 8 + 4 + 64)
/* max number of open files */
#define FILE_COUNT	(PROC_COUNT * 16)

/* an entry in the global file table */
typedef struct {
	/* may be (read OR write) AND (virtual OR real); flags = 0 => entry unused */
	u8 flags;
	/* number of references */
	u16 refCount;
	/* current position in file */
	u32 position;
	/* node-number; if virtual: index in nodes, if real: id in fs */
	u32 nodeNo;
} tGFTEntry;

/* all nodes */
static tVFSNode nodes[NODE_COUNT];
/* a pointer to the first free node (which points to the next and so on) */
static tVFSNode *freeList;

/* global file table */
static tGFTEntry globalFileTable[FILE_COUNT];

/**
 * Requests a new node and returns the pointer to it. Panics if there are no free nodes anymore.
 *
 * @return the pointer to the node
 */
static tVFSNode *vfs_requestNode(void) {
	tVFSNode *node;
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
static void vfs_releaseNode(tVFSNode *node) {
	node->next = freeList;
	freeList = node;
}

/**
 * The recursive function to print the VFS-tree
 *
 * @param level the current recursion level
 * @param parent the parent node
 */
static void vfs_doPrintTree(u32 level,tVFSNode *parent) {
	u32 i;
	tVFSNode *n = parent->childs;
	while(n != NULL) {
		for(i = 0;i < level;i++)
			vid_printf(" |");
		vid_printf("- %s\n",n->name);
		vfs_doPrintTree(level + 1,n);
		n = n->next;
	}
}

void vfs_init(void) {
	u32 i;
	tVFSNode *root,*sys,*node = &nodes[0];
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

tVFSNode *vfs_getNode(u32 index) {
	return nodes + index;
}

s32 vfs_openFile(u8 flags,u32 nodeNo) {
	s32 i,freeSlot = ERR_NO_FREE_FD;
	bool write = flags & GFT_WRITE;
	u8 mode = flags & (GFT_REAL | GFT_VIRTUAL);
	tGFTEntry *e = &globalFileTable[0];

	/* ensure that we don't increment usages of an unused slot */
	if(flags == 0)
		panic("No flags given");

	for(i = 0; i < FILE_COUNT; i++) {
		/* used slot and same flags & node? */
		if(e->flags != 0) {
			if((e->flags & (GFT_REAL | GFT_VIRTUAL)) == mode && e->nodeNo == nodeNo) {
				/* writing to the same file is not possible */
				if(write && e->flags & GFT_WRITE)
					return ERR_FILE_IN_USE;

				/* if the flags are different we need a different slot */
				if(e->flags == flags) {
					e->refCount++;
					return i;
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
		e = &globalFileTable[freeSlot];
		e->flags = flags;
		e->refCount = 1;
		e->position = 0;
		e->nodeNo = nodeNo;
	}

	return freeSlot;
}

void vfs_closeFile(u32 fd) {
	/* invalid file-descriptor? */
	if(fd >= FILE_COUNT)
		return;

	/* not in use? */
	if(globalFileTable[fd].flags == 0)
		return;

	/* decrement references */
	if(--(globalFileTable[fd].refCount) == 0) {
		/* mark unused */
		globalFileTable[fd].flags = 0;
	}
}

void vfs_printGFT(void) {
	u32 i;
	tGFTEntry *e = globalFileTable;
	vid_printf("Global File Table:\n");
	for(i = 0; i < FILE_COUNT; i++) {
		if(e->flags != 0) {
			vid_printf("\tfile @ index %d\n",i);
			vid_printf("\t\tread: %d\n",(e->flags & GFT_READ) ? true : false);
			vid_printf("\t\twrite: %d\n",(e->flags & GFT_WRITE) ? true : false);
			vid_printf("\t\tenv: %s\n",(e->flags & GFT_VIRTUAL) ? "virtual" : "real");
			vid_printf("\t\tnodeNo: %d\n",e->nodeNo);
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

void vfs_printNode(tVFSNode *node) {
	vid_printf("VFSNode @ 0x%x:\n",node);
	if(node) {
		vid_printf("\tname: %s\n",node->name);
		vid_printf("\ttype: %s\n",node->type == T_DIR ? "DIR" : node->type == T_INFO ? "INFO" : "SERVICE");
		vid_printf("\tchilds: 0x%x\n",node->childs);
		vid_printf("\tnext: 0x%x\n",node->next);
		vid_printf("\tdata: 0x%x\n",node->data);
		vid_printf("\tdataSize: %d\n",node->dataSize);
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

s32 vfs_resolvePath(cstring path,bool *isReal) {
	tVFSNode *n;
	s32 pos;
	/* select start */
	if(*path == '/') {
		n = &nodes[0];
		path++;
	}
	else
		panic("TODO: use current path");

	/* root/current node requested? */
	if(!*path)
		return ((u32)n - (u32)&nodes[0]) / sizeof(tVFSNode);

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
				*isReal = true;
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

	*isReal = false;
	/* return index */
	return ((u32)n - (u32)&nodes[0]) / sizeof(tVFSNode);
}

tVFSNode *vfs_createDir(tVFSNode *parent,tVFSNode *prev,string name) {
	tVFSNode *node = vfs_createNode(parent,prev,name);
	node->type = T_DIR;
	node->dataSize = 0;
	node->data = NULL;
	return node;
}

tVFSNode *vfs_createInfo(tVFSNode *parent,tVFSNode *prev,string name,void *data,u16 dataSize) {
	tVFSNode *node = vfs_createNode(parent,prev,name);
	node->type = T_INFO;
	node->dataSize = dataSize;
	node->data = data;
	return node;
}

tVFSNode *vfs_createService(tVFSNode *parent,tVFSNode *prev,string name) {
	tVFSNode *node = vfs_createNode(parent,prev,name);
	/* TODO */
	node->type = T_SERVICE;
	node->dataSize = 0;
	node->data = NULL;
	return node;
}

tVFSNode *vfs_createNode(tVFSNode *parent,tVFSNode *prev,string name) {
	tVFSNode *node = vfs_requestNode();
	node->name = name;
	node->next = NULL;
	node->childs = NULL;

	if(prev != NULL)
		prev->next = node;
	else if(parent != NULL)
		parent->childs = node;
	return node;
}
