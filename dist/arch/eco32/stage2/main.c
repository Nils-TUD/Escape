/**
 * $Id: main.c 52 2009-08-13 12:39:24Z nasmussen $
 * Copyright (C) 2009 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/debug.h>
#include <sys/arch/eco32/boot.h>
#include "../../../../drivers/common/fs/ext2/ext2.h"
#include <stdarg.h>

#define SEC_SIZE			512								/* disk sector size in bytes */
#define SPB(e)				(EXT2_BLK_SIZE(e) / SEC_SIZE)	/* sectors per block */

/* hardcoded here; the monitor will choose them later */
#define BOOT_DISK		0
#define START_SECTOR	8		/* part 0 */
#define SECTOR_COUNT	17912	/* size of part 0 */

/**
 * Reads/Writes from/to disk
 */
extern int dskio(int dskno, char cmd, int sct, unsigned int addr, int nscts);
extern int sctcapctl(void);

/* the tasks we should load */
static sLoadProg progs[PROG_COUNT] = {
	{"/boot/kernel.bin",BL_K_ID,0,0,0},
	{"/boot/hdd.bin",BL_HDD_ID,0,0,0},
	{"/boot/fs.bin",BL_FS_ID,0,0,0},
	{"/boot/rtc.bin",BL_RTC_ID,0,0,0}
};

static sExt2 super;
static sExt2Inode rootIno;
/* temporary buffer to read a block from disk */
static char buffer[1024];
/* the start-address for loading programs; the bootloader needs 1 page for data and 1 stack-page */
static uint loadAddr = 0xC0000000;

static void *memcpy(void *dst,const void *src,size_t count) {
	void *res = dst;
	while(count-- > 0)
		*(char*)dst++ = *(char*)src++;
	return res;
}

void halt(const char *s,...) {
	va_list ap;
	va_start(ap,s);
	debugf("Error: ");
	vdebugf(s,ap);
	va_end(ap);
	while(1);
}

#if 0
void printInode(D_INODE *ip) {
	int i;
	debugf("mode = %x\n",ip->mode);
	debugf("nlink = %d\n",ip->nlink);
	debugf("uid = %d\n",ip->uid);
	debugf("gid = %d\n",ip->gid);
	debugf("size = %d\n",ip->size);
	debugf("atime = %d\n",ip->atime);
	debugf("mtime = %d\n",ip->mtime);
	debugf("ctime = %d\n",ip->ctime);
	debugf("direct:\n");
	for(i = 0; i < DIRECT; i++)
		debugf("\t%d: %d\n",i,ip->direct[i]);
	debugf("indir1 = %d\n",ip->indir1);
	debugf("indir2 = %d\n",ip->indir2);
	debugf("indir3 = %d\n",ip->indir3);
}
#endif

void readFromDisk(tBlockNo blkno,char *buf,uint offset,uint nbytes) {
	unsigned int sector;
	int result;

	if(offset >= EXT2_BLK_SIZE(&super) || offset + nbytes > EXT2_BLK_SIZE(&super))
		halt("Offset / nbytes invalid");

	sector = blkno * SPB(&super);
	if(sector + SPB(&super) > SECTOR_COUNT)
		halt("Sector number exceeds disk or partition size");
	/*debugf("Reading sectors %d .. %d\n",sector + START_SECTOR,sector + START_SECTOR + SPB);*/
	result = dskio(BOOT_DISK,'r',sector + START_SECTOR,(uint)buffer & 0x3FFFFFFF,SPB(&super));
	if(result != 0)
		halt("Load error");

	memcpy(buf,buffer + offset,nbytes);
}

#if 0
void loadInode(D_INODE *ip,ushort inodeno) {
	BLKNO blkno;
	uint iindex;

	blkno = FIRST_IBLK + (inodeno - 1) / INO_PER_BLK;
	iindex = ((inodeno - 1) % INO_PER_BLK);

	readFromDisk(blkno,(char *)ip,iindex * DINODE_SIZE,DINODE_SIZE);
}

BLKNO getBlock(D_INODE *ino,ulong offset) {
	BLKNO log_blk = (BLKNO)(offset / BLOCK_SIZE);
	BLKNO *phy_blk;
	ulong div;
	uint lvl,lindex;
	BLKNO *blp;

	/* determine level of indirection */
	if(log_blk >= INDIR2) {
		lvl = 4;
		phy_blk = &(ino->indir3);
		log_blk -= INDIR2;
		div = ((ulong)NO_PER_BLOCK) * ((ulong)NO_PER_BLOCK);
	}
	else if(log_blk >= INDIR1) {
		lvl = 3;
		phy_blk = &(ino->indir2);
		log_blk -= INDIR1;
		div = (ulong)NO_PER_BLOCK;
	}
	else if(log_blk >= DIRECT) {
		lvl = 2;
		phy_blk = &(ino->indir1);
		log_blk -= DIRECT;
		div = 1L;
	}
	else {
		lvl = 1;
		phy_blk = &ino->direct[log_blk];
	}

	while(--lvl) {
		readFromDisk(*phy_blk,buffer,0,BLOCK_SIZE);

		if(lvl == 0)
			break;

		blp = (BLKNO *)buffer;
		lindex = (uint)(log_blk / div);
		log_blk %= div;
		div /= (ulong)NO_PER_BLOCK;
		phy_blk = &blp[lindex];
	}

	return *phy_blk;
}

ushort searchDir(D_INODE *ip,char *name) {
	ushort ino = 0;
	ulong offset = 0L;
	BLKNO blkno;
	char *cp,*limit;

	while(offset < ip->size) {
		blkno = getBlock(ip,offset);
		readFromDisk(blkno,buffer,0,BLOCK_SIZE);

		if(offset + BLOCK_SIZE < ip->size)
			limit = buffer + BLOCK_SIZE;
		else
			limit = buffer + (int)(ip->size - offset);

		for(cp = buffer; cp < limit; cp += DIR_SIZE,offset += DIR_SIZE) {
			/* free slot */
			if(*((ushort *)cp) == 0)
				continue;

			/* found it */
			if(memcmp(cp + 2,name,14) == 0) {
				ino = *((ushort *)cp);
				break; /* leave for loop   */
			}
		}

		if(ino)
			break; /* leave while loop */
	}
	return ino;
}

ushort namei(char *path,D_INODE *ip) {
	ushort inodeno;
	char *p = path;
	char *q;
	char name[14];
	int i;
	ulong offset;

	/* no relative paths */
	if(*path != '/')
		return 0;

	inodeno = ROOT_INO;
	memcpy(ip,&rootIno,sizeof(D_INODE));

	/* root? */
	if(*path == '/' && *(path + 1) == '\0')
		return 0;

	if(*p == '/')
		++p;

	while(*p) {
		/* read next path name component from input */
		for(i = 0,q = name; *p && *p != '/' && i < 14; ++i)
			*q++ = *p++;
		while(i++ < 14)
			*q++ = ' ';
		if(*p == '/')
			++p;

		if((ip->mode & IFMT) != IFDIR)
			return 0;

		inodeno = searchDir(ip,name);
		/* component not found */
		if(inodeno == 0)
			return 0;

		loadInode(ip,inodeno);
	}
	return inodeno;
}

void copyToMem(D_INODE *ip,uint offset,uint count) {
	BLKNO blk;
	uint offInBlk,amount;
	while(count > 0) {
		blk = getBlock(ip,offset);

		offInBlk = offset % BLOCK_SIZE;
		amount = MIN(count,BLOCK_SIZE - offInBlk);
		readFromDisk(blk,(char*)loadAddr,offInBlk,amount);

		count -= amount;
		offset += amount;
		loadAddr += amount;
		/* printing a loading-'status' lowers the subjective wait-time ^^ */
		dbg_printc('.');
	}
}

void loadProg(sLoadProg *prog,D_INODE *ip) {
	uint offset;
	ExecHeader header;

	/* make page-boundary */
	loadAddr = (loadAddr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	prog->start = loadAddr;

	/* read header */
	readFromDisk(ip->direct[0],(char*)&header,0,sizeof(ExecHeader));

	if(header.magic != EXEC_MAGIC)
		halt("Invalid magic-number in %s",prog->path);

	/* copy code */
	offset = sizeof(ExecHeader);
	copyToMem(ip,offset,header.csize);

	/* copy data to next page */
	loadAddr = (loadAddr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	offset += header.csize;
	copyToMem(ip,offset,header.dsize);

	/* clear bss */
	offset += header.dsize;
	memclear((void*)loadAddr,header.bsize);
	loadAddr += header.bsize;

	/* store sizes */
	prog->codeSize = header.csize;
	prog->dataSize = header.dsize + header.bsize;
}
#endif

sLoadProg *bootload(void) {
	int i;
	int cap = sctcapctl();
	if(cap == 0)
		halt("Disk not found");

	/* load superblock */
	dskio(BOOT_DISK,'r',EXT2_SUPERBLOCK_SECNO,(uint)&super,2);

#if 0
	/* load root-inode */
	loadInode(&rootIno,ROOT_INO);

	for(i = 0; i < PROG_COUNT; i++) {
		/* get inode of path */
		if(namei(progs[i].path,&ip) == 0)
			halt("Unable to find '%s'\n",progs[i].path);
		else {
			/* load program */
			debugf("Loading %s",progs[i].path);
			loadProg(progs + i,&ip);
			debugf("\n");

			if(progs[i].id == BL_K_ID) {
				/* give the kernel one page for the initial stack */
				loadAddr = (loadAddr + PAGE_SIZE * 2 - 1) & ~(PAGE_SIZE - 1);
			}
		}
	}
#endif

	return progs;
}
