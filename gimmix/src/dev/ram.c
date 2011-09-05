/**
 * $Id: ram.c 218 2011-06-04 15:46:40Z nasmussen $
 */

#include <stdlib.h>

#include "common.h"
#include "core/bus.h"
#include "dev/ram.h"
#include "mmix/io.h"
#include "mmix/mem.h"
#include "exception.h"
#include "config.h"
#include "sim.h"

// Use a treap for the physical memory to be able to have a lot of main-memory but only pay
// for it, if it is used. A treap is a combination of a binary tree and a heap. So the child-node
// on the left has a smaller key (addr) than the parent and the child on the right has a bigger key.
// Additionally the root-node has the smallest priority and it increases when walking towards the
// leafs. The priority is "randomized" by fibonacci-hashing. This way, the tree is well balanced
// in most cases.

#define MEMNODE_SIZE 2048

typedef struct sMemNode {
	octa addr;
	tetra priority;
	tetra buffer[MEMNODE_SIZE / sizeof(tetra)];
	struct sMemNode *left;
	struct sMemNode *right;
} sMemNode;

static void ram_reset(void);
static void ram_load(const char *file);
static void ram_free(sMemNode *n);
static void ram_shutdown(void);
static octa ram_read(octa addr,bool sideEffects);
static void ram_write(octa addr,octa value);
static void ram_doPrint(const sMemNode *n,int layer);
static tetra *ram_find(octa addr);
static sMemNode *ram_createNode(void);

static sMemNode *root;
static sMemNode *last;
static tetra priority;
static sDevice ramDevice;

void ram_init(void) {
	ramDevice.name = "RAM";
	ramDevice.startAddr = RAM_START_ADDR;
	ramDevice.endAddr = cfg_getRAMSize() - 1;
	ramDevice.irqMask = 0;
	ramDevice.reset = ram_reset;
	ramDevice.shutdown = ram_shutdown;
	ramDevice.read = ram_read;
	ramDevice.write = ram_write;
	bus_register(&ramDevice);
	ram_reset();
}

static void ram_reset(void) {
	ram_free(root);
	root = last = NULL;
	priority = 314159265;
	if(cfg_getProgram())
		ram_load(cfg_getProgram());
}

static void ram_load(const char *file) {
	FILE *f = fopen(file,"r");
	if(!f)
		sim_error("Unable to load program-file '%s'",file);
	octa loc = 0;
	while(!feof(f)) {
		int c = fgetc(f);
		if(c == EOF)
			break;
		if(c == '\n')
			continue;
		if(c != ' ') {
			ungetc(c,f);
			if(mfscanf(f,"%Ox:",&loc) != 1)
				sim_error("Invalid program-file '%s'",file);
			// alignment is required when accessing pm
			loc &= ~(octa)(sizeof(octa) - 1);
		}
		else {
			octa o;
			if(mfscanf(f,"%Ox",&o) != 1)
				sim_error("Invalid program-file '%s'",file);
			bus_write(loc,o,false);
			loc += sizeof(loc);
		}
		// walk to line end
		while(!feof(f) && fgetc(f) != '\n');
	}
	fclose(f);
}

static void ram_free(sMemNode *n) {
	if(n) {
		ram_free(n->left);
		ram_free(n->right);
	}
	mem_free(n);
}

static void ram_shutdown(void) {
	ram_free(root);
}

static octa ram_read(octa addr,bool sideEffects) {
	UNUSED(sideEffects);
	if(addr >= cfg_getRAMSize())
		ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);

	// creating nodes is not considered critical here for side-effects. the software won't
	// recognize it.
	tetra *m = ram_find(addr);
	octa value = (octa)*m << 32;
	m = ram_find(addr + sizeof(tetra));
	value |= *m;
	return value;
}

static void ram_write(octa addr,octa value) {
	if(addr >= cfg_getRAMSize())
		ex_throw(EX_DYNAMIC_TRAP,TRAP_NONEX_MEMORY);

	tetra *m = ram_find(addr);
	*m = value >> 32;
	m = ram_find(addr + sizeof(tetra));
	*m = value & 0xFFFFFFFF;
}

void ram_print(void) {
	ram_doPrint(root,0);
	mprintf("\n");
}

static void ram_doPrint(const sMemNode *n,int layer) {
	if(n) {
		mprintf("prio=%08x, addr=%Ox\n",n->priority,n->addr);
		mprintf("%*s|-(l) ",layer * 2,"");
		ram_doPrint(n->left,layer + 1);
		mprintf("\n");
		mprintf("%*s|-(r) ",layer * 2,"");
		ram_doPrint(n->right,layer + 1);
	}
}

static tetra *ram_find(octa addr) {
	octa key = addr & ~(octa)(MEMNODE_SIZE - 1);
	int offset = addr & (MEMNODE_SIZE - 1);
	sMemNode *p = last;
	// no root-node yet?
	if(!p) {
		root = last = ram_createNode();
		root->addr = addr;
		return &root->buffer[offset / sizeof(tetra)];
	}

	if(p->addr != key) {
		// search in tree
		for(p = root; p != NULL; ) {
			if(p->addr == key) {
				// found!
				last = p;
				return &p->buffer[offset / sizeof(tetra)];
			}
			if(key < p->addr)
				p = p->left;
			else
				p = p->right;
		}
		// not found => find a place for a new node
		// we want to insert it by priority, so find the first node that has <= priority
		sMemNode **q;
		for(p = root, q = &root; p && p->priority < priority; p = *q) {
			if(key < p->addr)
				q = &p->left;
			else
				q = &p->right;
		}
		// create new node
		*q = ram_createNode();
		(*q)->addr = key;
		// At this point we want to split the binary search tree p into two parts based on the
		// given key, forming the left and right subtrees of the new node q. The effect will be
		// as if key had been inserted before all of p’s nodes.
		sMemNode **l = &(*q)->left, **r = &(*q)->right;
		while(p) {
			if(key < p->addr) {
				*r = p;
				r = &p->left;
				p = *r;
			}
			else {
				*l = p;
				l = &p->right;
				p = *l;
			}
		}
		*l = *r = NULL;
		p = *q;
		last = p;
	}
	return &p->buffer[offset / sizeof(tetra)];
}

static sMemNode *ram_createNode(void) {
	sMemNode *n = mem_calloc(1,sizeof(sMemNode));
	// fibonacci hashing to spread the priorities very even in the 32-bit room
	n->priority = priority;
	priority += 0x9e3779b9;	// floor(2^32 / phi), with phi = golden ratio
	return n;
}
