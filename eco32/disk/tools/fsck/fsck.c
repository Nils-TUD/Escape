/*
 * fsck.c -- check an EOS32 file system
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


/**************************************************************/


#define SSIZE		512	/* disk sector size in bytes */
#define BSIZE		4096	/* disk block size in bytes */
#define SPB		(BSIZE / SSIZE)

#define NICINOD		500	/* number of inodes in superblock */
#define NICFREE		500	/* number of free blocks in superblock */
#define NADDR		8	/* number of block addresses in inode */
#define NDADDR		6	/* number of direct block addresses */
#define SINGLE_INDIR	(NDADDR + 0)	/* index of single indirect block */
#define DOUBLE_INDIR	(NDADDR + 1)	/* index of double indirect block */
#define DIRSIZ		60	/* max length of a path name component */

#define NIPB		(BSIZE / sizeof(Dinode))
#define NDIRENT		(BSIZE / sizeof(Dirent))
#define NINDIR		(BSIZE / sizeof(EOS32_daddr_t))

#define SUPER_MAGIC	0x44FCB67D

#define IFMT		070000	/* type of file */
#define   IFREG		040000	/* regular file */
#define   IFDIR		030000	/* directory */
#define   IFCHR		020000	/* character special */
#define   IFBLK		010000	/* block special */
#define   IFFREE	000000	/* reserved (indicates free inode) */
#define ISUID		004000	/* set user id on execution */
#define ISGID		002000	/* set group id on execution */
#define ISVTX		001000	/* save swapped text even after use */
#define IUREAD		000400	/* user's read permission */
#define IUWRITE		000200	/* user's write permission */
#define IUEXEC		000100	/* user's execute permission */
#define IGREAD		000040	/* group's read permission */
#define IGWRITE		000020	/* group's write permission */
#define IGEXEC		000010	/* group's execute permission */
#define IOREAD		000004	/* other's read permission */
#define IOWRITE		000002	/* other's write permission */
#define IOEXEC		000001	/* other's execute permission */

/*
 * The following two macros convert an inode number to the disk
 * block containing the inode and an inode number within the block.
 */
#define itod(i)		(2 + (i) / NIPB)
#define itoo(i)		((i) % NIPB)

#define TYPE_COPY	0
#define TYPE_INODE	1
#define TYPE_FREE	2
#define TYPE_SUPER	3
#define TYPE_INDIRECT	4
#define TYPE_DIRECTORY	5

typedef int Bool;
#define FALSE		0
#define TRUE		1


/**************************************************************/


/* EOS32 types */

typedef unsigned long EOS32_ino_t;
typedef unsigned long EOS32_daddr_t;
typedef unsigned long EOS32_off_t;
typedef long EOS32_time_t;


/* super block */

typedef struct {
  unsigned int s_magic;			/* must be SUPER_MAGIC */
  EOS32_daddr_t s_fsize;		/* size of file system in blocks */
  EOS32_daddr_t s_isize;		/* size of inode list in blocks */
  EOS32_daddr_t s_freeblks;		/* number of free blocks */
  EOS32_ino_t s_freeinos;		/* number of free inodes */
  unsigned int s_ninode;		/* number of inodes in s_inode */
  EOS32_ino_t s_inode[NICINOD];		/* free inode list */
  unsigned int s_nfree;			/* number of addresses in s_free */
  EOS32_daddr_t s_free[NICFREE];	/* free block list */
  EOS32_time_t s_time;			/* last super block update */
  char s_flock;				/* lock for free list manipulation */
  char s_ilock;				/* lock for i-list manipulation */
  char s_fmod;				/* super block modified flag */
  char s_ronly;				/* mounted read-only flag */
  char s_pad[BSIZE - 4036];		/* pad to block size */
} Filsys;


/* inode on disk */

typedef struct {
  unsigned int di_mode;			/* type and mode of file */
  unsigned int di_nlink;		/* number of links to file */
  unsigned int di_uid;			/* owner's user id */
  unsigned int di_gid;			/* owner's group id */
  EOS32_time_t di_ctime;		/* time created */
  EOS32_time_t di_mtime;		/* time last modified */
  EOS32_time_t di_atime;		/* time last accessed */
  EOS32_off_t di_size;			/* number of bytes in file */
  EOS32_daddr_t di_addr[NADDR];		/* block addresses */
} Dinode;


/* directory entry */

typedef struct {
  EOS32_ino_t d_ino;			/* directory inode */
  char d_name[DIRSIZ];			/* directory name */
} Dirent;


/* free block */

typedef struct {
  unsigned int df_nfree;		/* number of valid block addresses */
  EOS32_daddr_t df_free[NICFREE];	/* addresses of free blocks */
  char df_pad[BSIZE - 2004];		/* pad to block size */
} Fblk;


/**************************************************************/


Bool autoRepair;
FILE *disk;
Filsys filsys;
EOS32_daddr_t firstDataBlock;


typedef struct {
  int used;		/* how many times is the block used? */
  int free;		/* how many times is the block on the free list? */
} BlockInfo;

BlockInfo *blockInfo;
EOS32_daddr_t numBlocks;


typedef struct {
  int links;		/* how many links are recorded in the inode? */
  int refs;		/* how man times is the inode referenced? */
} InodeInfo;

InodeInfo *inodeInfo;
EOS32_ino_t numInodes;


/**************************************************************/


void error(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  printf("Error: ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
  exit(1);
}


void *allocMemory(unsigned int size) {
  void *p;

  if (size == 0) {
    error("attempt to allocate 0 bytes of memory");
  }
  p = malloc(size);
  if (p == NULL) {
    error("out of memory");
  }
  return p;
}


void freeMemory(void *p) {
  if (p == NULL) {
    error("attempt to free NULL referenced memory");
  }
  free(p);
}


/**************************************************************/


unsigned int read4FromEco(unsigned char *p) {
  return (unsigned int) p[0] << 24 |
         (unsigned int) p[1] << 16 |
         (unsigned int) p[2] <<  8 |
         (unsigned int) p[3] <<  0;
}


void write4ToEco(unsigned char *p, unsigned int data) {
  p[0] = data >> 24;
  p[1] = data >> 16;
  p[2] = data >>  8;
  p[3] = data >>  0;
}


void conv4FromEcoToX86(unsigned char *p) {
  unsigned int data;

  data = read4FromEco(p);
  * (unsigned int *) p = data;
}


void conv4FromX86ToEco(unsigned char *p) {
  unsigned int data;

  data = * (unsigned int *) p;
  write4ToEco(p, data);
}


/**************************************************************/


void superFromEcoToX86(unsigned char *p) {
  int i;

  /* magic */
  conv4FromEcoToX86(p);
  p += 4;
  /* fsize */
  conv4FromEcoToX86(p);
  p += 4;
  /* isize */
  conv4FromEcoToX86(p);
  p += 4;
  /* freeblks */
  conv4FromEcoToX86(p);
  p += 4;
  /* freeinos */
  conv4FromEcoToX86(p);
  p += 4;
  /* ninode */
  conv4FromEcoToX86(p);
  p += 4;
  /* inode */
  for (i = 0; i < NICINOD; i++) {
    conv4FromEcoToX86(p);
    p += 4;
  }
  /* nfree */
  conv4FromEcoToX86(p);
  p += 4;
  /* free */
  for (i = 0; i < NICFREE; i++) {
    conv4FromEcoToX86(p);
    p += 4;
  }
  /* time */
  conv4FromEcoToX86(p);
  p += 4;
  /* flock */
  p += 1;
  /* ilock */
  p += 1;
  /* fmod */
  p += 1;
  /* ronly */
  p += 1;
}


void superFromX86ToEco(unsigned char *p) {
  int i;

  /* magic */
  conv4FromX86ToEco(p);
  p += 4;
  /* fsize */
  conv4FromX86ToEco(p);
  p += 4;
  /* isize */
  conv4FromX86ToEco(p);
  p += 4;
  /* freeblks */
  conv4FromX86ToEco(p);
  p += 4;
  /* freeinos */
  conv4FromX86ToEco(p);
  p += 4;
  /* ninode */
  conv4FromX86ToEco(p);
  p += 4;
  /* inode */
  for (i = 0; i < NICINOD; i++) {
    conv4FromX86ToEco(p);
    p += 4;
  }
  /* nfree */
  conv4FromX86ToEco(p);
  p += 4;
  /* free */
  for (i = 0; i < NICFREE; i++) {
    conv4FromX86ToEco(p);
    p += 4;
  }
  /* time */
  conv4FromX86ToEco(p);
  p += 4;
  /* flock */
  p += 1;
  /* ilock */
  p += 1;
  /* fmod */
  p += 1;
  /* ronly */
  p += 1;
}


void inodeFromEcoToX86(unsigned char *p) {
  int i, j;

  for (i = 0; i < NIPB; i++) {
    /* mode */
    conv4FromEcoToX86(p);
    p += 4;
    /* nlink */
    conv4FromEcoToX86(p);
    p += 4;
    /* uid */
    conv4FromEcoToX86(p);
    p += 4;
    /* gid */
    conv4FromEcoToX86(p);
    p += 4;
    /* ctime */
    conv4FromEcoToX86(p);
    p += 4;
    /* mtime */
    conv4FromEcoToX86(p);
    p += 4;
    /* atime */
    conv4FromEcoToX86(p);
    p += 4;
    /* size */
    conv4FromEcoToX86(p);
    p += 4;
    /* addr */
    for (j = 0; j < NADDR; j++) {
      conv4FromEcoToX86(p);
      p += 4;
    }
  }
}


void inodeFromX86ToEco(unsigned char *p) {
  int i, j;

  for (i = 0; i < NIPB; i++) {
    /* mode */
    conv4FromX86ToEco(p);
    p += 4;
    /* nlink */
    conv4FromX86ToEco(p);
    p += 4;
    /* uid */
    conv4FromX86ToEco(p);
    p += 4;
    /* gid */
    conv4FromX86ToEco(p);
    p += 4;
    /* ctime */
    conv4FromX86ToEco(p);
    p += 4;
    /* mtime */
    conv4FromX86ToEco(p);
    p += 4;
    /* atime */
    conv4FromX86ToEco(p);
    p += 4;
    /* size */
    conv4FromX86ToEco(p);
    p += 4;
    /* addr */
    for (j = 0; j < NADDR; j++) {
      conv4FromX86ToEco(p);
      p += 4;
    }
  }
}


void freeFromEcoToX86(unsigned char *p) {
  int i;

  /* nfree */
  conv4FromEcoToX86(p);
  p += 4;
  /* free */
  for (i = 0; i < NICFREE; i++) {
    conv4FromEcoToX86(p);
    p += 4;
  }
}


void freeFromX86ToEco(unsigned char *p) {
  int i;

  /* nfree */
  conv4FromX86ToEco(p);
  p += 4;
  /* free */
  for (i = 0; i < NICFREE; i++) {
    conv4FromX86ToEco(p);
    p += 4;
  }
}


void indirectFromEcoToX86(unsigned char *p) {
  int i;

  for (i = 0; i < NINDIR; i++) {
    conv4FromEcoToX86(p);
    p += 4;
  }
}


void indirectFromX86ToEco(unsigned char *p) {
  int i;

  for (i = 0; i < NINDIR; i++) {
    conv4FromX86ToEco(p);
    p += 4;
  }
}


void directoryFromEcoToX86(unsigned char *p) {
  int i;

  for (i = 0; i < NDIRENT; i++) {
    conv4FromEcoToX86(p);
    p += 4;
    p += DIRSIZ;
  }
}


void directoryFromX86ToEco(unsigned char *p) {
  int i;

  for (i = 0; i < NDIRENT; i++) {
    conv4FromX86ToEco(p);
    p += 4;
    p += DIRSIZ;
  }
}


/**************************************************************/


unsigned long fsStart;		/* file system start sector */
unsigned long fsSize;		/* file system size in sectors */


void rdfs(EOS32_daddr_t bno, unsigned char *bf, int blkType) {
  int n;

  if (bno * SPB >= fsSize) {
    error("read request for block %ld is outside of file system", bno);
  }
  fseek(disk, fsStart * SSIZE + bno * BSIZE, SEEK_SET);
  n = fread(bf, 1, BSIZE, disk);
  if (n != BSIZE) {
    error("file system read error on block %ld", bno);
  }
  switch (blkType) {
    case TYPE_COPY:
      /* nothing to do here */
      break;
    case TYPE_INODE:
      inodeFromEcoToX86(bf);
      break;
    case TYPE_FREE:
      freeFromEcoToX86(bf);
      break;
    case TYPE_SUPER:
      superFromEcoToX86(bf);
      break;
    case TYPE_INDIRECT:
      indirectFromEcoToX86(bf);
      break;
    case TYPE_DIRECTORY:
      directoryFromEcoToX86(bf);
      break;
    default:
      error("illegal block type %d in rdfs()", blkType);
      break;
  }
}


void wtfs(EOS32_daddr_t bno, unsigned char *bf, int blkType) {
  int n;

  if (bno * SPB >= fsSize) {
    error("write request for block %ld is outside of file system", bno);
  }
  switch (blkType) {
    case TYPE_COPY:
      /* nothing to do here */
      break;
    case TYPE_INODE:
      inodeFromX86ToEco(bf);
      break;
    case TYPE_FREE:
      freeFromX86ToEco(bf);
      break;
    case TYPE_SUPER:
      superFromX86ToEco(bf);
      break;
    case TYPE_INDIRECT:
      indirectFromX86ToEco(bf);
      break;
    case TYPE_DIRECTORY:
      directoryFromX86ToEco(bf);
      break;
    default:
      error("illegal block type %d in wtfs()", blkType);
      break;
  }
  fseek(disk, fsStart * SSIZE + bno * BSIZE, SEEK_SET);
  n = fwrite(bf, 1, BSIZE, disk);
  if(n != BSIZE) {
    error("file system write error on block %ld", bno);
  }
  switch (blkType) {
    case TYPE_COPY:
      /* nothing to do here */
      break;
    case TYPE_INODE:
      inodeFromEcoToX86(bf);
      break;
    case TYPE_FREE:
      freeFromEcoToX86(bf);
      break;
    case TYPE_SUPER:
      superFromEcoToX86(bf);
      break;
    case TYPE_INDIRECT:
      indirectFromEcoToX86(bf);
      break;
    case TYPE_DIRECTORY:
      directoryFromEcoToX86(bf);
      break;
    default:
      error("illegal block type %d in wtfs()", blkType);
      break;
  }
}


/**************************************************************/


#define LINE_SIZE	200


Bool askUser(char *fmt, ...) {
  va_list ap;
  char line[LINE_SIZE];

  if (autoRepair) {
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    return TRUE;
  }
  while(1) {
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf(", repair? (y/n/q) ");
    fflush(stdout);
    if (fgets(line, LINE_SIZE, stdin) == NULL) {
      printf("\n");
      exit(0);
    }
    if (line[0] == 'q' || line[0] == 'Q') {
      printf("\n");
      exit(0);
    }
    if (line[0] == 'y' || line[0] == 'n' ||
        line[0] == 'Y' || line[0] == 'N') {
      if (line[1] == '\n' || line[1] == '\0') {
        break;
      }
    }
  }
  return line[0] == 'y' ||
         line[0] == 'Y';
}


/**************************************************************/


void freeBlock(EOS32_daddr_t bno) {
  Fblk fbuf;
  int i;

  if (filsys.s_nfree > NICFREE) {
    error("this should never happen (filsys.s_nfree > NICFREE)");
  }
  if (filsys.s_nfree == NICFREE) {
    memset(&fbuf, 0, sizeof(Fblk));
    fbuf.df_nfree = filsys.s_nfree;
    for (i = 0; i < NICFREE; i++) {
      fbuf.df_free[i] = filsys.s_free[i];
    }
    wtfs(bno, (unsigned char *) &fbuf, TYPE_FREE);
    filsys.s_nfree = 0;
  }
  filsys.s_free[filsys.s_nfree++] = bno;
  if (bno != 0) {
    /* remark: block number 0 does not count as free block */
    filsys.s_freeblks++;
  }
}


void rebuildFreeList(void) {
  int i;
  EOS32_daddr_t bno;

  filsys.s_nfree = 0;
  for (i = 0; i < NICFREE; i++) {
    filsys.s_free[i] = 0;
  }
  filsys.s_freeblks = 0;
  freeBlock(0);
  for (bno = numBlocks - 1; bno >= firstDataBlock; bno--) {
    if (blockInfo[bno].used == 0) {
      freeBlock(bno);
    }
  }
}


/**************************************************************/


void checkBlockNumber(EOS32_daddr_t n, EOS32_ino_t ino,
                      Dinode *iptr, Bool *dirty) {
  if (n < firstDataBlock || n >= numBlocks) {
    if (askUser("inode %lu (0x%lX) references illegal block %lu (0x%lX)",
                ino, ino, n, n)) {
      /* free inode */
      iptr->di_mode &= ~IFMT;
      *dirty = TRUE;
    }
  }
}


void countBlocksInFile(EOS32_ino_t ino, Dinode *iptr, Bool *dirty) {
  EOS32_off_t size;
  EOS32_daddr_t nBlocks, blkno, n;
  EOS32_daddr_t siBlock[NINDIR];
  EOS32_daddr_t siBlkno;
  EOS32_daddr_t diBlock[NINDIR];
  EOS32_daddr_t diBlkno;

  size = iptr->di_size;
  nBlocks = (size + BSIZE - 1) / BSIZE;
  siBlkno = 0;
  diBlkno = 0;
  for (blkno = 0; blkno < nBlocks; blkno++) {
    if (blkno < NDADDR) {
      /* direct block */
      n = iptr->di_addr[blkno];
      checkBlockNumber(n, ino, iptr, dirty);
      blockInfo[n].used++;
    } else
    if (blkno < NDADDR + NINDIR) {
      /* single indirect block */
      n = iptr->di_addr[SINGLE_INDIR];
      checkBlockNumber(n, ino, iptr, dirty);
      if (siBlkno != n) {
        rdfs(n, (unsigned char *) siBlock, TYPE_INDIRECT);
        siBlkno = n;
        blockInfo[n].used++;
      }
      n = siBlock[blkno - NDADDR];
      checkBlockNumber(n, ino, iptr, dirty);
      blockInfo[n].used++;
    } else {
      /* double indirect block */
      n = iptr->di_addr[DOUBLE_INDIR];
      checkBlockNumber(n, ino, iptr, dirty);
      if (diBlkno != n) {
        rdfs(n, (unsigned char *) diBlock, TYPE_INDIRECT);
        diBlkno = n;
        blockInfo[n].used++;
      }
      n = diBlock[(blkno - NDADDR - NINDIR) / NINDIR];
      checkBlockNumber(n, ino, iptr, dirty);
      if (siBlkno != n) {
        rdfs(n, (unsigned char *) siBlock, TYPE_INDIRECT);
        siBlkno = n;
        blockInfo[n].used++;
      }
      n = siBlock[(blkno - NDADDR - NINDIR) % NINDIR];
      checkBlockNumber(n, ino, iptr, dirty);
      blockInfo[n].used++;
    }
  }
}


void countBlocksInInodes(void) {
  EOS32_daddr_t bno;
  Dinode inodes[NIPB];
  Bool dirty;
  int i;
  Dinode *iptr;
  EOS32_ino_t ino;

  for (bno = 0; bno < numBlocks; bno++) {
    blockInfo[bno].used = 0;
  }
  for (bno = 0; bno < filsys.s_isize; bno++) {
    rdfs(2 + bno, (unsigned char *) inodes, TYPE_INODE);
    dirty = FALSE;
    for (i = 0; i < NIPB; i++) {
      iptr = &inodes[i];
      ino = bno * NIPB + i;
      switch (iptr->di_mode & IFMT) {
        case IFREG:
          countBlocksInFile(ino, iptr, &dirty);
          break;
        case IFDIR:
          countBlocksInFile(ino, iptr, &dirty);
          break;
        case IFCHR:
          /* no blocks allocated */
          break;
        case IFBLK:
          /* no blocks allocated */
          break;
        case IFFREE:
          /* no blocks allocated */
          break;
        default:
          if (askUser("inode %lu (0x%lX) has illegal type %lu (0x%lX)",
              ino, ino, iptr->di_mode, iptr->di_mode)) {
            /* free inode */
            iptr->di_mode &= ~IFMT;
            dirty = TRUE;
          }
          break;
      }
    }
    if (dirty) {
      wtfs(2 + bno, (unsigned char *) inodes, TYPE_INODE);
    }
  }
}


void countBlocksInFreeList(void) {
  EOS32_daddr_t bno;
  int nfree, i;
  EOS32_daddr_t free[NICFREE];
  Fblk fbuf;

  while (1) {
    for (bno = 0; bno < numBlocks; bno++) {
      blockInfo[bno].free = 0;
    }
    nfree = filsys.s_nfree;
    for (i = 0; i < NICFREE; i++) {
      free[i] = filsys.s_free[i];
    }
    while (1) {
      bno = free[--nfree];
      if (bno == 0) {
        return;
      }
      if (bno < firstDataBlock || bno >= numBlocks) {
        if (askUser("free list contains illegal block number %lu (0x%lX)",
                    bno, bno)) {
          /* rebuild free list and start all over again */
          rebuildFreeList();
          break;
        }
      }
      blockInfo[bno].free++;
      if (nfree == 0) {
        rdfs(bno, (unsigned char *) &fbuf, TYPE_FREE);
        nfree = fbuf.df_nfree;
        for (i = 0; i < NICFREE; i++) {
          free[i] = fbuf.df_free[i];
        }
      }
    }
  }
}


void checkBlocks(void) {
  EOS32_daddr_t blk;

  printf("Checking blocks...\n");
  while (1) {
    /* walk the inodes for blocks */
    countBlocksInInodes();
    /* walk the free list for blocks */
    countBlocksInFreeList();
    /* now check */
    for (blk = firstDataBlock; blk < numBlocks; blk++) {
      if (blockInfo[blk].used == 1 && blockInfo[blk].free == 0) {
        /* ok, block is allocated */
      } else
      if (blockInfo[blk].used == 0 && blockInfo[blk].free == 1) {
        /* ok, block is free */
      } else
      if (blockInfo[blk].used == 0 && blockInfo[blk].free == 0) {
        /* block is missing */
        if (askUser("block %lu (0x%lX) is missing", blk, blk)) {
          /* add to free list */
          freeBlock(blk);
        }
      } else
      if (blockInfo[blk].used == 1 && blockInfo[blk].free == 1) {
        /* block is allocated and free */
        if (askUser("block %lu (0x%lX) is allocated AND free", blk, blk)) {
          /* rebuild free list and start all over again */
          rebuildFreeList();
          break;
        }
      } else
      if (blockInfo[blk].free > 1) {
        /* block is multiply in free list */
        if (askUser("block %lu (0x%lX) is multiply in free list", blk, blk)) {
          /* rebuild free list and start all over again */
          rebuildFreeList();
          break;
        }
      } else
      if (blockInfo[blk].used > 1) {
        /* block is multiply allocated */
        if (askUser("block %lu (0x%lX) is multiply allocated", blk, blk)) {
          /* difficult */
          error("difficult, not yet implemented");
        }
      }
    }
    if (blk == numBlocks) {
      break;
    }
  }
}


/**************************************************************/


void getLinksFromInodes(void) {
  EOS32_daddr_t bno;
  Dinode inodes[NIPB];
  int i;
  Dinode *iptr;
  EOS32_ino_t ino;

  for (bno = 0; bno < filsys.s_isize; bno++) {
    rdfs(2 + bno, (unsigned char *) inodes, TYPE_INODE);
    for (i = 0; i < NIPB; i++) {
      iptr = &inodes[i];
      ino = bno * NIPB + i;
      if ((iptr->di_mode & IFMT) == IFFREE) {
        inodeInfo[ino].links = 0;
      } else {
        inodeInfo[ino].links = iptr->di_nlink;
      }
    }
  }
}


Dirent *loadDir(Dinode *iptr, EOS32_off_t dirOff) {
  EOS32_daddr_t blkno;
  EOS32_daddr_t n;
  static EOS32_daddr_t dirBlkno = 0;
  static Dirent dirBlock[NDIRENT];
  static EOS32_daddr_t siBlkno = 0;
  static EOS32_daddr_t siBlock[NINDIR];
  static EOS32_daddr_t diBlkno = 0;
  static EOS32_daddr_t diBlock[NINDIR];

  blkno = dirOff / BSIZE;
  if (blkno < NDADDR) {
    /* direct block */
    n = iptr->di_addr[blkno];
  } else
  if (blkno < NDADDR + NINDIR) {
    /* single indirect block */
    n = iptr->di_addr[SINGLE_INDIR];
    if (siBlkno != n) {
      rdfs(n, (unsigned char *) siBlock, TYPE_INDIRECT);
      siBlkno = n;
    }
    n = siBlock[blkno - NDADDR];
  } else {
    /* double indirect block */
    n = iptr->di_addr[DOUBLE_INDIR];
    if (diBlkno != n) {
      rdfs(n, (unsigned char *) diBlock, TYPE_INDIRECT);
      diBlkno = n;
    }
    n = diBlock[(blkno - NDADDR - NINDIR) / NINDIR];
    if (siBlkno != n) {
      rdfs(n, (unsigned char *) siBlock, TYPE_INDIRECT);
      siBlkno = n;
    }
    n = siBlock[(blkno - NDADDR - NINDIR) % NINDIR];
  }
  if (dirBlkno != n) {
    rdfs(n, (unsigned char *) dirBlock, TYPE_DIRECTORY);
    dirBlkno = n;
  }
  return &dirBlock[(dirOff % BSIZE) / sizeof(Dirent)];
}


void countLinksRecursively(EOS32_ino_t ino) {
  unsigned char inoBuf[BSIZE];
  Dinode *iptr;
  EOS32_off_t dirOff;
  Dirent *dptr;

  inodeInfo[ino].refs++;
  rdfs(itod(ino), inoBuf, TYPE_INODE);
  iptr = (Dinode *) inoBuf + itoo(ino);
  switch (iptr->di_mode & IFMT) {
    case IFREG:
      /* no further inodes referenced from here */
      break;
    case IFDIR:
      /* descend recursively */
      for (dirOff = 0; dirOff < iptr->di_size; dirOff += sizeof(Dirent)) {
        dptr = loadDir(iptr, dirOff);
        if (dptr->d_ino != 0) {
          /* directory entry is used */
          if (dptr->d_ino >= numInodes) {
            /* this is an illegal inode number */
            if (askUser("directory entry references an illegal inode")) {
              /* clear directory entry */
              dptr->d_ino = 0;
              /* !!!! must write directory data block back !!!! */
            }
          } else {
            /* this is a legal inode number */
            if (dirOff == 0 * sizeof(Dirent)) {
              /* count, but do not follow '.' */
              inodeInfo[dptr->d_ino].refs++;
            } else
            if (dirOff == 1 * sizeof(Dirent)) {
              /* count, but do not follow '..' */
              inodeInfo[dptr->d_ino].refs++;
            } else {
              /* follow the directory entry */
              countLinksRecursively(dptr->d_ino);
            }
          }
        }
      }
      break;
    case IFCHR:
      /* no further inodes referenced from here */
      break;
    case IFBLK:
      /* no further inodes referenced from here */
      break;
    case IFFREE:
      /* no further inodes referenced from here */
      break;
    default:
      error("inode %lu (0x%lX) has illegal type %lu (0x%lX)",
            ino, ino, iptr->di_mode, iptr->di_mode);
      break;
  }
}


void checkFiles(void) {
  EOS32_ino_t ino;

  printf("Checking files...\n");
  while (1) {
    /* walk the inodes for link counts */
    getLinksFromInodes();
    /* walk the directories for link counts */
    for (ino = 0; ino < numInodes; ino++) {
      inodeInfo[ino].refs = 0;
    }
    countLinksRecursively(1);
    inodeInfo[1].refs--;
    /* now check */
    for (ino = 0; ino < numInodes; ino++) {
      if (inodeInfo[ino].links == inodeInfo[ino].refs) {
        /* ok, link count and actual number of references agree */
      } else
      if (inodeInfo[ino].links > inodeInfo[ino].refs) {
        error("links > refs");
      } else
      if (inodeInfo[ino].links < inodeInfo[ino].refs) {
        error("links < refs");
      }
    }
    if (ino == numInodes) {
      break;
    }
  }
}


/**************************************************************/


void usage(char *myself) {
  printf("Usage: %s <disk> <partition> [-a]\n", myself);
  printf("       <disk> is a disk image file name\n");
  printf("       <partition> is a partition number ");
  printf("(or '*' for the whole disk)\n");
  printf("       -a tries to repair a damaged file system automatically\n");
  exit(1);
}


int main(int argc, char *argv[]) {
  int part;
  char *endptr;
  unsigned char partTable[SSIZE];
  unsigned char *ptptr;
  unsigned long partType;
  EOS32_daddr_t maxBlocks;

  autoRepair = FALSE;
  if (argc != 3 && argc != 4) {
    usage(argv[0]);
  }
  if (argc == 4) {
    if (strcmp(argv[3], "-a") == 0) {
      autoRepair = TRUE;
    } else {
      usage(argv[0]);
    }
  }
  disk = fopen(argv[1], "r+b");
  if (disk == NULL) {
    error("cannot open disk image file '%s'", argv[1]);
  }
  if (strcmp(argv[2], "*") == 0) {
    /* whole disk contains one single file system */
    fsStart = 0;
    fseek(disk, 0, SEEK_END);
    fsSize = ftell(disk) / SSIZE;
  } else {
    /* argv[2] is partition number of file system */
    part = strtoul(argv[2], &endptr, 10);
    if (*endptr != '\0' || part < 0 || part > 15) {
      error("illegal partition number '%s'", argv[2]);
    }
    fseek(disk, 1 * SSIZE, SEEK_SET);
    if (fread(partTable, 1, SSIZE, disk) != SSIZE) {
      error("cannot read partition table of disk '%s'", argv[1]);
    }
    ptptr = partTable + part * 32;
    partType = read4FromEco(ptptr + 0);
    if ((partType & 0x7FFFFFFF) != 0x00000058) {
      error("partition %d of disk '%s' does not contain an EOS32 file system",
            part, argv[1]);
    }
    fsStart = read4FromEco(ptptr + 4);
    fsSize = read4FromEco(ptptr + 8);
  }
  if (fsSize % SPB != 0) {
    printf("Warning: disk/partition size is not a multiple of block size\n");
  }
  maxBlocks = fsSize / SPB;
  if (maxBlocks < 4) {
    error("disk/partition size is less than 4 blocks");
  }
  rdfs(1, (unsigned char *) &filsys, TYPE_SUPER);
  if (filsys.s_magic != SUPER_MAGIC) {
    error("wrong magic number in super block");
  }
  if (filsys.s_fsize < 4) {
    error("file system size is less than 4 blocks");
  }
  if (filsys.s_fsize > maxBlocks) {
    error("file system size exceeds disk/partition space");
  }
  if (filsys.s_fsize < maxBlocks) {
    printf("Warning: file system does not fully utilize available space\n");
  }
  firstDataBlock = 2 + filsys.s_isize;
  if (firstDataBlock >= filsys.s_fsize) {
    error("file system has no data blocks");
  }
  numBlocks = filsys.s_fsize;
  numInodes = filsys.s_isize * NIPB;
  printf("File system uses %lu (0x%lX) of %lu (0x%lX) blocks.\n",
         numBlocks, numBlocks, maxBlocks, maxBlocks);
  printf("File system can hold %lu (0x%lX) files.\n",
         numInodes, numInodes);
  blockInfo = allocMemory(numBlocks * sizeof(BlockInfo));
  inodeInfo = allocMemory(numInodes * sizeof(InodeInfo));
  checkBlocks();
  checkFiles();
  return 0;
}
