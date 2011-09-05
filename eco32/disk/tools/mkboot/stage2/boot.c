/*
 * boot.c -- second stage bootstrap (the boot loader)
 */


#include "iolib.h"


/**************************************************************/

/* constants and data types */


#define DEFAULT_KERNEL	"/boot/eos32.bin"    /* path of default OS kernel */

#define EXEC_MAGIC	0x3AE82DD4	/* magic number for executables */

#define PSHIFT		12		/* ld of page size */
#define PSIZE		(1 << PSHIFT)	/* page size in bytes */
#define PMASK		(PSIZE - 1)	/* page mask */

#define BSHIFT		12		/* ld of block size */
#define BSIZE		(1 << BSHIFT)	/* disk block size in bytes */
#define BMASK		(BSIZE - 1)	/* block mask */

#define SSIZE		512		/* disk sector size in bytes */
#define SPB		(BSIZE / SSIZE)	/* sectors per block */

#define ROOT_INO	1	/* inode number of root directory */
#define NADDR		8	/* number of block addresses in inode */
#define NDADDR		6	/* number of direct block addresses */
#define DIRSIZ		60	/* max length of path name component */

#define NIPB		(BSIZE / sizeof(Dinode))
#define NDIRENT		(BSIZE / sizeof(Dirent))
#define NINDIR		(BSIZE / sizeof(EOS32_daddr_t))

#define NULL		0


typedef unsigned long EOS32_ino_t;
typedef unsigned long EOS32_daddr_t;
typedef unsigned long EOS32_off_t;
typedef long EOS32_time_t;


#define IFMT		070000	/* type of file */
#define	  IFREG		040000	/* regular file */
#define	  IFDIR		030000	/* directory */
#define	  IFCHR		020000	/* character special */
#define	  IFBLK		010000	/* block special */
#define	  IFFREE	000000	/* reserved (indicates free inode) */
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


typedef struct {
  unsigned int di_mode;		/* type and mode of file */
  unsigned int di_nlink;	/* number of links to file */
  unsigned int di_uid;		/* owner's user id */
  unsigned int di_gid;		/* owner's group id */
  EOS32_time_t di_ctime;	/* time created */
  EOS32_time_t di_mtime;	/* time last modified */
  EOS32_time_t di_atime;	/* time last accessed */
  EOS32_off_t di_size;		/* number of bytes in file */
  EOS32_daddr_t di_addr[NADDR];	/* disk block addresses */
} Dinode;


typedef struct {
  EOS32_ino_t d_ino;		/* directory inode */
  char d_name[DIRSIZ];		/* directory name */
} Dirent;


/**************************************************************/

/* string functions */


int strlen(char *str) {
  int i;

  i = 0;
  while (*str++ != '\0') {
    i++;
  }
  return i;
}


void strcpy(char *dst, char *src) {
  while ((*dst++ = *src++) != '\0') ;
}


void memcpy(unsigned char *dst, unsigned char *src, unsigned int cnt) {
  while (cnt--) {
    *dst++ = *src++;
  }
}


/**************************************************************/

/* terminal I/O */


char getchar(void) {
  return getc();
}


void putchar(char c) {
  if (c == '\n') {
    putchar('\r');
  }
  putc(c);
}


void puts(char *s) {
  while (*s != '\0') {
    putchar(*s++);
  }
}


/**************************************************************/

/* get a line from the terminal */


char line[80];


void getLine(char *prompt) {
  int index;
  char c;

  puts(prompt);
  puts(line);
  index = strlen(line);
  while (1) {
    c = getchar();
    switch (c) {
      case '\r':
        putchar('\n');
        line[index] = '\0';
        return;
      case '\b':
      case 0x7F:
        if (index == 0) {
          break;
        }
        putchar('\b');
        putchar(' ');
        putchar('\b');
        index--;
        break;
      default:
        if (c == '\t') {
          c = ' ';
        }
        if (c < 0x20 || c > 0x7E) {
          break;
        }
        putchar(c);
        line[index++] = c;
        break;
    }
  }
}


/**************************************************************/

/* halt with error message */


void halt(char *msg) {
  puts(msg);
  puts(", machine halted.\n");
  while (1) ;
}


/**************************************************************/

/* disk I/O */


unsigned int bootDisk = 0;	/* gets loaded by previous stage */
unsigned int startSector = 0;	/* gets loaded by previous stage */
unsigned int numSectors = 0;	/* gets loaded by previous stage */


void readBlock(unsigned int blkno, unsigned char *buffer) {
  unsigned int sector;
  int result;

  sector = blkno * SPB;
  if (sector + SPB > numSectors) {
    halt("Sector number exceeds disk or partition size");
  }
  result = rwscts(bootDisk, 'r', sector + startSector,
                  (unsigned int) buffer & 0x3FFFFFFF, SPB);
  if (result != 0) {
    halt("Load error");
  }
}


EOS32_daddr_t sibn = -1;
EOS32_daddr_t singleIndirectBlock[NINDIR];
EOS32_daddr_t dibn = -1;
EOS32_daddr_t doubleIndirectBlock[NINDIR];


void readFileBlock(Dinode *inode, int blkno, unsigned char *addr) {
  EOS32_daddr_t diskBlock;

  if (blkno < NDADDR) {
    /* direct block */
    diskBlock = inode->di_addr[blkno];
  } else
  if (blkno < NDADDR + NINDIR) {
    /* single indirect block */
    diskBlock = inode->di_addr[NDADDR];
    if (sibn != diskBlock) {
      readBlock(diskBlock, (unsigned char *) singleIndirectBlock);
      sibn = diskBlock;
    }
    diskBlock = singleIndirectBlock[blkno - NDADDR];
  } else {
    /* double indirect block */
    diskBlock = inode->di_addr[NDADDR + 1];
    if (dibn != diskBlock) {
      readBlock(diskBlock, (unsigned char *) doubleIndirectBlock);
      dibn = diskBlock;
    }
    diskBlock = doubleIndirectBlock[(blkno - NDADDR - NINDIR) / NINDIR];
    if (sibn != diskBlock) {
      readBlock(diskBlock, (unsigned char *) singleIndirectBlock);
      sibn = diskBlock;
    }
    diskBlock = singleIndirectBlock[(blkno - NDADDR - NINDIR) % NINDIR];
  }
  readBlock(diskBlock, addr);
}


/**************************************************************/

/* inode handling */


Dinode inodeBlock[NIPB];
Dirent directoryBlock[NDIRENT];


Dinode *iget(unsigned int inode) {
  unsigned int bnum;
  unsigned int inum;

  bnum = itod(inode);
  inum = itoo(inode);
  readBlock(bnum, (unsigned char *) inodeBlock);
  return &inodeBlock[inum];
}


Dinode *namei(char *path) {
  Dinode *dp;
  char c;
  char dirBuffer[DIRSIZ];
  char *cp;
  EOS32_off_t offset;
  Dirent dirEntry;
  int i;

  dp = iget(ROOT_INO);
  c = *path++;
  while (c == '/') {
    c = *path++;
  }
  if (c == '\0') {
    return NULL;
  }
  while (1) {
    /* here dp contains pointer to last component matched */
    if (c == '\0') {
      /* no more path components */
      break;
    }
    /* if there is another component, gather up name */
    cp = dirBuffer;
    while (c != '/' && c != '\0') {
      if (cp < &dirBuffer[DIRSIZ]) {
        *cp++ = c;
      }
      c = *path++;
    }
    while (cp < &dirBuffer[DIRSIZ]) {
      *cp++ = '\0';
    }
    while (c == '/') {
      c = *path++;
    }
    /* dp must be a directory */
    if ((dp->di_mode & IFMT) != IFDIR) {
      return NULL;
    }
    /* search directory */
    offset = 0;
    while (1) {
      if (offset >= dp->di_size) {
        /* not found */
        return NULL;
      }
      if ((offset & BMASK) == 0) {
        /* read the next directory block */
        readFileBlock(dp, offset >> BSHIFT,
                      (unsigned char *) directoryBlock);
      }
      memcpy((unsigned char *) &dirEntry,
             (unsigned char *) directoryBlock + (offset & BMASK),
             sizeof(Dirent));
      offset += sizeof(Dirent);
      if (dirEntry.d_ino == 0) {
        /* directory slot is empty */
        continue;
      }
      for (i = 0; i < DIRSIZ; i++) {
        if (dirBuffer[i] != dirEntry.d_name[i]) {
          break;
        }
      }
      if (i == DIRSIZ) {
        /* component matched */
        break;
      }
      /* no match, try next entry */
    }
    /* get corresponding inode */
    dp = iget(dirEntry.d_ino);
    /* look for more components */
  }
  return dp;
}


/**************************************************************/

/* main program */


int main(void) {
  Dinode *fileInode;
  int numBlocks;
  int nextBlock;
  unsigned char *nextAddress;

  puts("Bootstrap loader executing...\n");
  strcpy(line, DEFAULT_KERNEL);
  while (1) {
    getLine("\nPlease enter path to kernel: ");
    fileInode = namei(line);
    if (fileInode == NULL) {
      puts(line);
      puts(" not found\n");
      continue;
    }
    if ((fileInode->di_mode & IFMT) != IFREG) {
      puts(line);
      puts(" is not a regular file\n");
      continue;
    }
    break;
  }
  puts("Loading ");
  puts(line);
  puts("...\n");
  /* load file */
  numBlocks = (fileInode->di_size + (BSIZE - 1)) / BSIZE;
  nextBlock = 0;
  nextAddress = (unsigned char *) 0xC0000000;
  while (numBlocks--) {
    readFileBlock(fileInode, nextBlock, nextAddress);
    nextBlock++;
    nextAddress += BSIZE;
  }
  puts("Starting ");
  puts(line);
  puts("...\n");
  return 0;
}
