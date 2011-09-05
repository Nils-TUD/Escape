/*
 * mkfs.c -- make an EOS32 file system
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>


/**************************************************************/


#define SSIZE		512	/* disk sector size in bytes */
#define BSIZE		4096	/* disk block size in bytes */
#define SPB		(BSIZE / SSIZE)	/* sectors per block */
#define AFS		(3 * BSIZE / 2)	/* average file size */

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


/* inode in memory */

typedef struct {
  EOS32_ino_t i_number;			/* inode number */
  unsigned int i_mode;			/* type and mode of file */
  unsigned int i_nlink;			/* number of links to file */
  unsigned int i_uid;			/* owner's user id */
  unsigned int i_gid;			/* owner's group id */
  EOS32_off_t i_size;			/* number of bytes in file */
  EOS32_daddr_t i_addr[NADDR];		/* block addresses */
} Inode;


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


time_t now;		/* timestamp used throughout the file system */
FILE *fs;		/* the file which holds the disk image */
Filsys filsys;		/* the file system's super block */
EOS32_ino_t lastIno;	/* last inode allocated */


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


void rdfs(EOS32_daddr_t bno, unsigned char *bf, int blkType) {
  int n;

  fseek(fs, fsStart * SSIZE + bno * BSIZE, SEEK_SET);
  n = fread(bf, 1, BSIZE, fs);
  if (n != BSIZE) {
    printf("read error: %ld\n", bno);
    exit(1);
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
  fseek(fs, fsStart * SSIZE + bno * BSIZE, SEEK_SET);
  n = fwrite(bf, 1, BSIZE, fs);
  if(n != BSIZE) {
    printf("write error: %ld\n", bno);
    exit(1);
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


char *charp;
FILE *fin;
char string[100];


char getChar(void) {
  int c;

  if (charp != NULL) {
    /* take character from string */
    c = *charp;
    if (c != 0) {
      charp++;
    }
  } else {
    /* take character from prototype file */
    c = getc(fin);
    if (c == EOF) {
      c = 0;
    }
  }
  return c;
}


void getStr(void) {
  char c;
  int i;

  while (1) {
    c = getChar();
    if (c == ' ' || c == '\t' || c == '\n') {
      continue;
    }
    if (c == '\0') {
      error("unexpected EOF in prototype file");
    }
    if (c == '#') {
      do {
        c = getChar();
        if (c == '\0') {
          error("unexpected EOF in prototype file");
        }
      } while (c != '\n');
      continue;
    }
    break;
  }
  i = 0;
  do {
    string[i++] = c;
    c = getChar();
  } while (c != ' ' &&
           c != '\t' &&
           c != '\n' &&
           c != '\0');
  string[i] = '\0';
}


void badMode(char c) {
  error("%c/%s: bad mode", c, string);
}


int getMode(char c, char *s, int m0, int m1, int m2, int m3) {
  if (s[0] == 0) badMode(c);
  if (c == s[0]) return m0;
  if (s[1] == 0) badMode(c);
  if (c == s[1]) return m1;
  if (s[2] == 0) badMode(c);
  if (c == s[2]) return m2;
  if (s[3] == 0) badMode(c);
  if (c == s[3]) return m3;
  badMode(c);
  /* never reached */
  return 0;
}


long getNum(void) {
  int i, c;
  long n;

  getStr();
  n = 0;
  i = 0;
  for (i = 0; (c = string[i]) != '\0'; i++) {
    if (c < '0' || c > '9') {
      error("%s: bad number", string);
    }
    n = n * 10 + (c - '0');
  }
  return n;
}


/**************************************************************/

/* data block handling */


EOS32_daddr_t allocBlock(void) {
  EOS32_daddr_t bno;
  Fblk fbuf;
  int i;

  if (filsys.s_nfree <= 0) {
    error("this should never happen (filsys.s_nfree <= 0)");
  }
  bno = filsys.s_free[--filsys.s_nfree];
  if (bno == 0) {
    error("out of free disk blocks");
  }
  if (filsys.s_nfree == 0) {
    rdfs(bno, (unsigned char *) &fbuf, TYPE_FREE);
    filsys.s_nfree = fbuf.df_nfree;
    for (i = 0; i < NICFREE; i++) {
      filsys.s_free[i] = fbuf.df_free[i];
    }
  }
  filsys.s_freeblks--;
  return bno;
}


void freeBlock(EOS32_daddr_t bno) {
  Fblk fbuf;
  int i;

  if (filsys.s_nfree > NICFREE) {
    error("this should never happen (filsys.s_nfree > NICFREE)");
  }
  if (filsys.s_nfree == NICFREE) {
    memset(&fbuf, 0, sizeof(Fblk));
    fbuf.df_nfree = NICFREE;
    for (i = 0; i < NICFREE; i++) {
      fbuf.df_free[i] = filsys.s_free[i];
    }
    wtfs(bno, (unsigned char *) &fbuf, TYPE_FREE);
    filsys.s_nfree = 0;
  }
  filsys.s_free[filsys.s_nfree++] = bno;
  if (bno != 0) {
    /* note: block number 0 does not count as free block */
    filsys.s_freeblks++;
  }
}


void initFreeList(void) {
  EOS32_daddr_t bno;

  freeBlock(0);
  for (bno = filsys.s_fsize - 1; bno >= 2 + filsys.s_isize; bno--) {
    freeBlock(bno);
  }
}


/**************************************************************/

/* inode handling */


void iput(Inode *ip) {
  EOS32_daddr_t inodeBlock;
  Dinode *dp;
  unsigned char buf[BSIZE];
  int i;

  inodeBlock = itod(ip->i_number);
  if (inodeBlock >= 2 + filsys.s_isize) {
    error("too few inodes");
  }
  rdfs(inodeBlock, buf, TYPE_INODE);
  dp = (Dinode *) buf;
  dp += itoo(ip->i_number);
  dp->di_mode = ip->i_mode;
  dp->di_nlink = ip->i_nlink;
  dp->di_uid = ip->i_uid;
  dp->di_gid = ip->i_gid;
  dp->di_ctime = now;
  dp->di_mtime = now;
  dp->di_atime = now;
  dp->di_size = ip->i_size;
  for (i = 0; i < NADDR; i++) {
    dp->di_addr[i] = ip->i_addr[i];
  }
  wtfs(inodeBlock, buf, TYPE_INODE);
  filsys.s_freeinos--;
}


void initInodes(void) {
  int i;
  EOS32_daddr_t n;
  unsigned char buf[BSIZE];
  Inode in;

  /* init all inodes to free */
  for (i = 0; i < BSIZE; i++) {
    buf[i] = '\0';
  }
  for (n = 0; n < filsys.s_isize; n++) {
    wtfs(2 + n, buf, TYPE_INODE);
    filsys.s_freeinos += NIPB;
  }
  /* init inode 0 */
  lastIno = 0;
  in.i_number = lastIno;
  in.i_mode = IFREG;
  in.i_nlink = 0;
  in.i_uid = 0;
  in.i_gid = 0;
  in.i_size = 0;
  for (i = 0; i < NADDR; i++) {
    in.i_addr[i] = 0;
  }
  iput(&in);
}


void fillFreeInodeList(EOS32_ino_t numInodes) {
  int index;
  EOS32_ino_t ino;
  int i;

  /* fill the free inode list starting at the upper end */
  index = NICINOD;
  for (ino = lastIno + 1; ino < numInodes && index > 0; ino++) {
    filsys.s_inode[--index] = ino;
  }
  filsys.s_ninode = NICINOD - index;
  /* if the list is not completely filled, shift entries down */
  if (index > 0) {
    for (i = 0; i < filsys.s_ninode; i++) {
      filsys.s_inode[i] = filsys.s_inode[index++];
    }
  }
}


/**************************************************************/


void appendData(Inode *ip, int blkType, unsigned char *src, int size) {
  EOS32_off_t offset;
  int index1, index2;
  EOS32_daddr_t bno;
  unsigned char buf[BSIZE];
  EOS32_daddr_t ibno;
  EOS32_daddr_t iblk[NINDIR];
  EOS32_daddr_t dibno;
  EOS32_daddr_t diblk[NINDIR];

  offset = ip->i_size;
  if (offset < NDADDR * BSIZE) {
    /* direct block */
    index1 = offset / BSIZE;
    offset %= BSIZE;
    if (ip->i_addr[index1] == 0) {
      bno = allocBlock();
      ip->i_addr[index1] = bno;
      memset(buf, 0, BSIZE);
    } else {
      bno = ip->i_addr[index1];
      rdfs(bno, buf, blkType);
    }
  } else
  if (offset < (NINDIR + NDADDR) * BSIZE) {
    /* single indirect block */
    offset -= NDADDR * BSIZE;
    index1 = offset / BSIZE;
    offset %= BSIZE;
    if (ip->i_addr[SINGLE_INDIR] == 0) {
      ibno = allocBlock();
      ip->i_addr[SINGLE_INDIR] = ibno;
      memset(iblk, 0, BSIZE);
    } else {
      ibno = ip->i_addr[SINGLE_INDIR];
      rdfs(ibno, (unsigned char *) iblk, TYPE_INDIRECT);
    }
    if (iblk[index1] == 0) {
      bno = allocBlock();
      iblk[index1] = bno;
      wtfs(ibno, (unsigned char *) iblk, TYPE_INDIRECT);
      memset(buf, 0, BSIZE);
    } else {
      bno = iblk[index1];
      rdfs(bno, buf, blkType);
    }
  } else {
    /* double indirect block */
    offset -= (NINDIR + NDADDR) * BSIZE;
    index1 = offset / BSIZE;
    offset %= BSIZE;
    index2 = index1 / NINDIR;
    index1 %= NINDIR;
    if (ip->i_addr[DOUBLE_INDIR] == 0) {
      dibno = allocBlock();
      ip->i_addr[DOUBLE_INDIR] = dibno;
      memset(diblk, 0, BSIZE);
    } else {
      dibno = ip->i_addr[DOUBLE_INDIR];
      rdfs(dibno, (unsigned char *) diblk, TYPE_INDIRECT);
    }
    if (diblk[index2] == 0) {
      ibno = allocBlock();
      diblk[index2] = ibno;
      wtfs(dibno, (unsigned char *) diblk, TYPE_INDIRECT);
      memset(iblk, 0, BSIZE);
    } else {
      ibno = diblk[index2];
      rdfs(ibno, (unsigned char *) iblk, TYPE_INDIRECT);
    }
    if (iblk[index1] == 0) {
      bno = allocBlock();
      iblk[index1] = bno;
      wtfs(ibno, (unsigned char *) iblk, TYPE_INDIRECT);
      memset(buf, 0, BSIZE);
    } else {
      bno = iblk[index1];
      rdfs(bno, buf, blkType);
    }
  }
  memcpy(buf + offset, src, size);
  wtfs(bno, buf, blkType);
  ip->i_size += size;
}


void makeDirEntry(Inode *dir, EOS32_ino_t ino, char *name) {
  Dirent entry;

  memset(&entry, 0, sizeof(Dirent));
  entry.d_ino = ino;
  strcpy(entry.d_name, name);
  appendData(dir, TYPE_DIRECTORY, (unsigned char *) &entry, sizeof(Dirent));
}


void createFile(Inode *parent) {
  Inode in;
  int i;
  char c;
  FILE *f;
  unsigned char buf[BSIZE];

  /* get mode, uid and gid */
  getStr();
  in.i_mode = getMode(string[0], "-bcd", IFREG, IFBLK, IFCHR, IFDIR);
  in.i_mode |= getMode(string[1], "-u", 0, ISUID, 0, 0);
  in.i_mode |= getMode(string[2], "-g", 0, ISGID, 0, 0);
  for (i = 3; i < 6; i++) {
    c = string[i];
    if (c < '0' || c > '7') {
      error("%c/%s: bad octal mode digit", c, string);
    }
    in.i_mode |= (c - '0') << (15 - 3 * i);
  }
  in.i_uid = getNum();
  in.i_gid = getNum();
  /* general initialization prior to switching on format */
  lastIno++;
  in.i_number = lastIno;
  in.i_nlink = 1;
  in.i_size = 0;
  for (i = 0; i < NADDR; i++) {
    in.i_addr[i] = 0;
  }
  if (parent == NULL) {
    parent = &in;
    in.i_nlink--;
  }
  /* now switch on format */
  switch (in.i_mode & IFMT) {
    case IFREG:
      /* regular file:
         contents is a file name */
      getStr();
      f = fopen(string, "rb");
      if (f == NULL) {
        error("%s: cannot open", string);
      }
      while ((i = fread(buf, 1, BSIZE, f)) > 0) {
        appendData(&in, TYPE_COPY, buf, i);
      }
      fclose(f);
      break;
    case IFBLK:
    case IFCHR:
      /* special file:
         contents is major/minor device */
      in.i_addr[0] = (getNum() & 0xFFFF) << 16;
      in.i_addr[0] |= (getNum() & 0xFFFF);
      break;
    case IFDIR:
      /* directory:
         put in extra links, call recursively until name is "$" */
      parent->i_nlink++;
      in.i_nlink++;
      makeDirEntry(&in, in.i_number, ".");
      makeDirEntry(&in, parent->i_number, "..");
      while (1) {
        getStr();
        if (string[0] == '$' && string[1] == '\0') {
          break;
        }
        makeDirEntry(&in, lastIno + 1, string);
        createFile(&in);
      }
      break;
    default:
      error("bad format/mode %o\n", in.i_mode);
      break;
  }
  /* finally write the inode */
  iput(&in);
}


/**************************************************************/


void showSizes(void) {
  printf("BSIZE   = %d\n", BSIZE);
  printf("NICINOD = %d\n", NICINOD);
  printf("NICFREE = %d\n", NICFREE);
  printf("NADDR   = %d\n", NADDR);
  printf("NDADDR  = %d\n", NDADDR);
  printf("DIRSIZ  = %d\n", DIRSIZ);
  printf("NIPB    = %d\n", NIPB);
  printf("NDIRENT = %d\n", NDIRENT);
  printf("NINDIR  = %d\n", NINDIR);
  printf("sizeof(Filsys) = %d\n", sizeof(Filsys));
  printf("sizeof(Dinode) = %d\n", sizeof(Dinode));
  printf("sizeof(Dirent) = %d\n", sizeof(Dirent));
  printf("sizeof(Fblk)   = %d\n", sizeof(Fblk));
}


int main(int argc, char *argv[]) {
  char *fsnam;
  char *proto;
  char protoBuf[20];
  unsigned long fsSize;
  int part;
  char *endptr;
  unsigned char partTable[SSIZE];
  unsigned char *ptptr;
  unsigned long partType;
  EOS32_daddr_t maxBlocks;
  EOS32_daddr_t numBlocks;
  EOS32_ino_t numInodes;
  int i;
  char c;
  FILE *bootblk;
  long bootblkSize;
  unsigned char buf[BSIZE];

  if (argc == 2 && strcmp(argv[1], "--sizes") == 0) {
    showSizes();
    exit(0);
  }
  if (argc != 3 && argc != 4) {
    printf("Usage: %s <disk> <partition or '*'> [<prototype or size>]\n",
           argv[0]);
    printf("   or: %s --sizes\n", argv[0]);
    exit(1);
  }
  time(&now);
  fsnam = argv[1];
  fs = fopen(fsnam, "r+b");
  if (fs == NULL) {
    error("cannot open disk image '%s'", fsnam);
  }
  if (strcmp(argv[2], "*") == 0) {
    /* whole disk contains one single file system */
    fsStart = 0;
    fseek(fs, 0, SEEK_END);
    fsSize = ftell(fs) / SSIZE;
  } else {
    /* argv[2] is partition number of file system */
    part = strtoul(argv[2], &endptr, 10);
    if (*endptr != '\0' || part < 0 || part > 15) {
      error("illegal partition number '%s'", argv[2]);
    }
    fseek(fs, 1 * SSIZE, SEEK_SET);
    if (fread(partTable, 1, SSIZE, fs) != SSIZE) {
      error("cannot read partition table of disk '%s'", fsnam);
    }
    ptptr = partTable + part * 32;
    partType = read4FromEco(ptptr + 0);
    if ((partType & 0x7FFFFFFF) != 0x00000058) {
      error("partition %d of disk '%s' does not contain an EOS32 file system",
            part, fsnam);
    }
    fsStart = read4FromEco(ptptr + 4);
    fsSize = read4FromEco(ptptr + 8);
  }
  printf("File system space is %lu (0x%lX) sectors of %d bytes each.\n",
         fsSize, fsSize, SSIZE);
  if (fsSize % SPB != 0) {
    printf("File system space is not a multiple of block size.\n");
  }
  maxBlocks = fsSize / SPB;
  printf("This equals %lu (0x%lX) blocks of %d bytes each.\n",
         maxBlocks, maxBlocks, BSIZE);
  if (argc == 4) {
    proto = argv[3];
  } else {
    proto = protoBuf;
    sprintf(protoBuf, "%lu", maxBlocks);
  }
  /* initialize super block and possibly write boot block */
  filsys.s_magic = SUPER_MAGIC;
  fin = fopen(proto, "rt");
  if (fin == NULL) {
    /* cannot open prototype file, perhaps size was specified */
    charp = "d--777 0 0 $ ";
    numBlocks = 0;
    for (i = 0; (c = proto[i]) != '\0'; i++) {
      if (c < '0' || c > '9') {
        /* neither valid prototype file nor valid size */
        error("cannot open prototype '%s'", proto);
      }
      numBlocks = numBlocks * 10 + (c - '0');
    }
    numInodes = (numBlocks * BSIZE) / AFS;
  } else {
    /* prototype file opened */
    charp = NULL;
    getStr();
    if (strcmp(string, "-noboot-") == 0) {
      /* boot block not wanted */
    } else {
      /* string holds the name of the boot block file */
      bootblk = fopen(string, "rb");
      if (bootblk == NULL) {
        error("cannot open boot block file '%s'", string);
      }
      fseek(bootblk, 0, SEEK_END);
      bootblkSize = ftell(bootblk);
      fseek(bootblk, 0, SEEK_SET);
      if (bootblkSize > BSIZE) {
        error("boot block file '%s' is bigger than a block", string);
      }
      for (i = 0; i < BSIZE; i++) {
        buf[i] = '\0';
      }
      if (fread(buf, 1, bootblkSize, bootblk) != bootblkSize) {
        error("cannot read boot block file '%s'", string);
      }
      wtfs(0, buf, TYPE_COPY);
      fclose(bootblk);
    }
    numBlocks = getNum();
    numInodes = getNum();
  }
  if (numBlocks > maxBlocks) {
    error("file system exceeds available space");
  }
  if (numBlocks < maxBlocks) {
    printf("File system does not fully utilize available space.\n");
  }
  filsys.s_fsize = numBlocks;
  filsys.s_isize = (numInodes + NIPB - 1) / NIPB;
  numInodes = filsys.s_isize * NIPB;
  if (2 + filsys.s_isize >= filsys.s_fsize) {
    error("bad block ratio (total/inode = %lu/%lu)",
          filsys.s_fsize, filsys.s_isize);
  }
  printf("File system size = %ld blocks\n", filsys.s_fsize);
  printf("Number of inodes = %ld inodes\n", numInodes);
  filsys.s_freeblks = 0;
  filsys.s_freeinos = 0;
  filsys.s_ninode = 0;
  for (i = 0; i < NICINOD; i++) {
    filsys.s_inode[i] = 0;
  }
  filsys.s_nfree = 0;
  for (i = 0; i < NICFREE; i++) {
    filsys.s_free[i] = 0;
  }
  filsys.s_time = now;
  filsys.s_flock = 0;
  filsys.s_ilock = 0;
  filsys.s_fmod = 0;
  filsys.s_ronly = 0;
  /* init inodes */
  initInodes();
  /* init free list */
  initFreeList();
  /* create files */
  createFile(NULL);
  /* fill free inode list in super block */
  /* note: this isn't strictly necessary, but cuts down file
     creation time within the newly created partition or disk */
  fillFreeInodeList(numInodes);
  /* finally write super block and return */
  wtfs(1, (unsigned char *) &filsys, TYPE_SUPER);
  return 0;
}
