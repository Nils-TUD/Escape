/*
 * shfs.c -- show an EOS32 file system
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>


#define SECTOR_SIZE	512	/* disk sector size in bytes */
#define BLOCK_SIZE	4096	/* disk block size in bytes */
#define SPB		(BLOCK_SIZE / SECTOR_SIZE)
#define LINE_SIZE	100	/* input line buffer size in bytes */
#define LINES_PER_BATCH	32	/* number of lines output in one batch */

#define NICINOD		500	/* number of free inodes in superblock */
#define NICFREE		500	/* number of free blocks in superblock */
#define INOPB		64	/* number of inodes per block */
#define DIRPB		64	/* number of directory entries per block */
#define DIRSIZ		60	/* max length of path name component */

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


typedef unsigned long EOS32_ino_t;
typedef unsigned long EOS32_daddr_t;
typedef unsigned long EOS32_off_t;
typedef long EOS32_time_t;


void error(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  printf("Error: ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
  exit(1);
}


unsigned long fsStart;		/* file system start sector */


void readBlock(FILE *disk,
               EOS32_daddr_t blockNum,
               unsigned char *blockBuffer) {
  fseek(disk, fsStart * SECTOR_SIZE + blockNum * BLOCK_SIZE, SEEK_SET);
  if (fread(blockBuffer, BLOCK_SIZE, 1, disk) != 1) {
    error("cannot read block %lu (0x%lX)", blockNum, blockNum);
  }
}


unsigned long get4Bytes(unsigned char *addr) {
  return (unsigned long) addr[0] << 24 |
         (unsigned long) addr[1] << 16 |
         (unsigned long) addr[2] <<  8 |
         (unsigned long) addr[3] <<  0;
}


int waitForReturn(void) {
  char line[LINE_SIZE];

  printf("press <enter> to continue, <esc> to break, q to quit: ");
  fflush(stdout);
  if (fgets(line, LINE_SIZE, stdin) == NULL) {
    printf("\n");
    exit(0);
  }
  if (line[0] == 'q') {
    exit(0);
  }
  if (line[0] == 0x1B) {
    return 1;
  }
  return 0;
}


int checkBatch(int numLines) {
  static int lines;
  int r;

  r = 0;
  if (numLines == 0) {
    /* initialize */
    lines = 0;
  } else {
    /* output numLines lines */
    lines += numLines;
    if (lines >= LINES_PER_BATCH) {
      r = waitForReturn();
      lines = 0;
    }
  }
  return r;
}


void rawBlock(unsigned char *p) {
  int i, j;
  unsigned char c;

  checkBatch(0);
  for (i = 0; i < BLOCK_SIZE / 16; i++) {
    printf("%04X   ", i * 16);
    for (j = 0; j < 16; j++) {
      c = p[i * 16 + j];
      printf("%02X ", c);
    }
    printf("   ");
    for (j = 0; j < 16; j++) {
      c = p[i * 16 + j];
      if (c < 0x20 || c >= 0x7F) {
        printf(".");
      } else {
        printf("%c", c);
      }
    }
    printf("\n");
    if (checkBatch(1)) return;
  }
}


void superBlock(unsigned char *p) {
  unsigned int magic;
  EOS32_daddr_t fsize;
  EOS32_daddr_t isize;
  EOS32_daddr_t freeblks;
  EOS32_ino_t freeinos;
  unsigned int ninode;
  EOS32_ino_t ino;
  unsigned int nfree;
  EOS32_daddr_t free;
  EOS32_time_t tim;
  char *dat;
  unsigned char flag;
  int i;

  checkBatch(0);
  magic = get4Bytes(p);
  p += 4;
  printf("magic number = %u (0x%08X)\n", magic, magic);
  if (checkBatch(1)) return;
  fsize = get4Bytes(p);
  p += 4;
  printf("file system size = %lu (0x%lX) blocks\n", fsize, fsize);
  if (checkBatch(1)) return;
  isize = get4Bytes(p);
  p += 4;
  printf("inode list size  = %lu (0x%lX) blocks\n", isize, isize);
  if (checkBatch(1)) return;
  freeblks = get4Bytes(p);
  p += 4;
  printf("free blocks = %lu (0x%lX)\n", freeblks, freeblks);
  if (checkBatch(1)) return;
  freeinos = get4Bytes(p);
  p += 4;
  printf("free inodes = %lu (0x%lX)\n", freeinos, freeinos);
  if (checkBatch(1)) return;
  ninode = get4Bytes(p);
  p += 4;
  printf("number of entries in free inode list = %u (0x%X)\n", ninode, ninode);
  if (checkBatch(1)) return;
  for (i = 0; i < NICINOD; i++) {
    ino = get4Bytes(p);
    p += 4;
    if (i < ninode) {
      printf("  free inode[%3d] = %lu (0x%lX)\n", i, ino, ino);
      if (checkBatch(1)) return;
    }
  }
  nfree = get4Bytes(p);
  p += 4;
  printf("number of entries in free block list = %u (0x%X)\n", nfree, nfree);
  if (checkBatch(1)) return;
  for (i = 0; i < NICFREE; i++) {
    free = get4Bytes(p);
    p += 4;
    if (i < nfree) {
      printf("  %s block[%3d] = %lu (0x%lX)\n",
             i == 0 ? "link" : "free", i, free, free);
      if (checkBatch(1)) return;
    }
  }
  tim = get4Bytes(p);
  p += 4;
  dat = ctime(&tim);
  dat[strlen(dat) - 1] = '\0';
  printf("last super block update = %ld (%s)\n", tim, dat);
  if (checkBatch(1)) return;
  flag = *p++;
  printf("free list locked flag = %d\n", (int) flag);
  if (checkBatch(1)) return;
  flag = *p++;
  printf("inode list locked flag = %d\n", (int) flag);
  if (checkBatch(1)) return;
  flag = *p++;
  printf("super block modified flag = %d\n", (int) flag);
  if (checkBatch(1)) return;
  flag = *p++;
  printf("fs mounted read-only flag = %d\n", (int) flag);
  if (checkBatch(1)) return;
}


void inodeBlock(unsigned char *p) {
  unsigned int mode;
  unsigned int nlink;
  unsigned int uid;
  unsigned int gid;
  EOS32_time_t tim;
  char *dat;
  EOS32_off_t size;
  EOS32_daddr_t addr;
  int i, j;

  checkBatch(0);
  for (i = 0; i < INOPB; i++) {
    printf("inode %d:", i);
    mode = get4Bytes(p);
    p += 4;
    if (mode != 0) {
      printf("  type/mode = 0x%08X = ", mode);
      if ((mode & IFMT) == IFREG) {
        printf("-");
      } else
      if ((mode & IFMT) == IFDIR) {
        printf("d");
      } else
      if ((mode & IFMT) == IFCHR) {
        printf("c");
      } else
      if ((mode & IFMT) == IFBLK) {
        printf("b");
      } else {
        printf("?");
      }
      printf("%c", mode & IUREAD  ? 'r' : '-');
      printf("%c", mode & IUWRITE ? 'w' : '-');
      printf("%c", mode & IUEXEC  ? 'x' : '-');
      printf("%c", mode & IGREAD  ? 'r' : '-');
      printf("%c", mode & IGWRITE ? 'w' : '-');
      printf("%c", mode & IGEXEC  ? 'x' : '-');
      printf("%c", mode & IOREAD  ? 'r' : '-');
      printf("%c", mode & IOWRITE ? 'w' : '-');
      printf("%c", mode & IOEXEC  ? 'x' : '-');
      if (mode & ISUID) {
        printf(", set uid");
      }
      if (mode & ISGID) {
        printf(", set gid");
      }
      if (mode & ISVTX) {
        printf(", save text");
      }
    } else {
      printf("  <free>");
    }
    printf("\n");
    if (checkBatch(1)) return;
    nlink = get4Bytes(p);
    p += 4;
    if (mode != 0) {
      printf("  nlnk = %u (0x%08X)\n", nlink, nlink);
      if (checkBatch(1)) return;
    }
    uid = get4Bytes(p);
    p += 4;
    if (mode != 0) {
      printf("  uid  = %u (0x%08X)\n", uid, uid);
      if (checkBatch(1)) return;
    }
    gid = get4Bytes(p);
    p += 4;
    if (mode != 0) {
      printf("  gid  = %u (0x%08X)\n", gid, gid);
      if (checkBatch(1)) return;
    }
    tim = get4Bytes(p);
    p += 4;
    dat = ctime(&tim);
    dat[strlen(dat) - 1] = '\0';
    if (mode != 0) {
      printf("  time inode created = %ld (%s)\n", tim, dat);
      if (checkBatch(1)) return;
    }
    tim = get4Bytes(p);
    p += 4;
    dat = ctime(&tim);
    dat[strlen(dat) - 1] = '\0';
    if (mode != 0) {
      printf("  time last modified = %ld (%s)\n", tim, dat);
      if (checkBatch(1)) return;
    }
    tim = get4Bytes(p);
    p += 4;
    dat = ctime(&tim);
    dat[strlen(dat) - 1] = '\0';
    if (mode != 0) {
      printf("  time last accessed = %ld (%s)\n", tim, dat);
      if (checkBatch(1)) return;
    }
    size = get4Bytes(p);
    p += 4;
    if (mode != 0) {
      printf("  size = %lu (0x%lX)\n", size, size);
      if (checkBatch(1)) return;
    }
    for (j = 0; j < 6; j++) {
      addr = get4Bytes(p);
      p += 4;
      if (mode != 0) {
        printf("  direct block[%1d] = %lu (0x%lX)\n", j, addr, addr);
        if (checkBatch(1)) return;
      }
    }
    addr = get4Bytes(p);
    p += 4;
    if (mode != 0) {
      printf("  single indirect = %lu (0x%lX)\n", addr, addr);
      if (checkBatch(1)) return;
    }
    addr = get4Bytes(p);
    p += 4;
    if (mode != 0) {
      printf("  double indirect = %lu (0x%lX)\n", addr, addr);
      if (checkBatch(1)) return;
    }
  }
}


void directoryBlock(unsigned char *p) {
  EOS32_ino_t ino;
  int i, j;
  unsigned char c;

  checkBatch(0);
  for (i = 0; i < DIRPB; i++) {
    printf("%02d:  ", i);
    ino = get4Bytes(p);
    p += 4;
    printf("inode = %lu (0x%lX)\n", ino, ino);
    if (checkBatch(1)) return;
    printf("     name  = ");
    if (*p == '\0') {
      printf("<empty>");
    } else {
      for (j = 0; j < DIRSIZ; j++) {
        c = *(p + j);
        if (c == '\0') {
          break;
        }
        if (c < 0x20 || c >= 0x7F) {
          printf(".");
        } else {
          printf("%c", c);
        }
      }
    }
    printf("\n");
    if (checkBatch(1)) return;
    p += DIRSIZ;
  }
}


void freeBlock(unsigned char *p) {
  unsigned int nfree;
  EOS32_daddr_t addr;
  int i;

  checkBatch(0);
  nfree = get4Bytes(p);
  p += 4;
  printf("nfree = %u (0x%X)\n", nfree, nfree);
  if (checkBatch(1)) return;
  for (i = 0; i < NICFREE; i++) {
    addr = get4Bytes(p);
    p += 4;
    if (i < nfree) {
      printf("  %s block[%3d] = %lu (0x%lX)\n",
             i == 0 ? "link" : "free", i, addr, addr);
      if (checkBatch(1)) return;
    }
  }
}


void indirectBlock(unsigned char *p) {
  EOS32_daddr_t addr;
  int i;

  checkBatch(0);
  for (i = 0; i < BLOCK_SIZE / sizeof(EOS32_daddr_t); i++) {
    addr = get4Bytes(p);
    p += 4;
    printf("block[%4d] = %lu (0x%lX)\n", i, addr, addr);
    if (checkBatch(1)) return;
  }
}


void help(void) {
  printf("Commands are:\n");
  printf("  h        help\n");
  printf("  q        quit\n");
  printf("  r        show block as raw data\n");
  printf("  s        show block as super block\n");
  printf("  i        show block as inode block\n");
  printf("  d        show block as directory block\n");
  printf("  f        show block as block on the free list\n");
  printf("  *        show block as indirect block\n");
  printf("  b <num>  set current block to <num>\n");
  printf("  +        increment current block\n");
  printf("  -        decrement current block\n");
  printf("  t <num>  translate inode <num> to block number\n");
  printf("           and relative inode number within block\n");
}


int parseNumber(char **pc, unsigned long *pi) {
  char *p;
  unsigned int base, dval;
  unsigned long n;

  p = *pc;
  while (*p == ' ' || *p == '\t') {
    p++;
  }
  if (*p == '\0' || *p == '\n') {
    printf("Error: number is missing!\n");
    return 0;
  }
  base = 10;
  if (*p == '0') {
    p++;
    if (*p != '\0' && *p != '\n') {
      if (*p == 'x' || *p == 'X') {
        base = 16;
        p++;
      } else {
        base = 8;
      }
    }
  }
  n = 0;
  while ((*p >= '0' && *p <= '9') ||
         (*p >= 'a' && *p <= 'f') ||
         (*p >= 'A' && *p <= 'F')) {
    if (*p >= '0' && *p <= '9') {
      dval = (*p - '0');
    } else
    if (*p >= 'a' && *p <= 'f') {
      dval = (*p - 'a' + 10);
    } else
    if (*p >= 'A' && *p <= 'F') {
      dval = (*p - 'A' + 10);
    }
    if (dval >= base) {
      printf("Error: digit value %d is illegal in number base %d\n",
             dval, base);
      return 0;
    }
    n *= base;
    n += dval;
    p++;
  }
  while (*p == ' ' || *p == '\t') {
    p++;
  }
  *pc = p;
  *pi = n;
  return 1;
}


int main(int argc, char *argv[]) {
  FILE *disk;
  unsigned long fsSize;
  int part;
  char *endptr;
  unsigned char partTable[SECTOR_SIZE];
  unsigned char *ptptr;
  unsigned long partType;
  EOS32_daddr_t numBlocks;
  EOS32_daddr_t currBlock;
  unsigned char blockBuffer[BLOCK_SIZE];
  char line[LINE_SIZE];
  int quit;
  char *p;
  unsigned long n;

  if (argc != 3) {
    printf("Usage: %s <disk> <partition>\n", argv[0]);
    printf("       <disk> is a disk image file name\n");
    printf("       <partition> is a partition number ");
    printf("(or '*' for the whole disk)\n");
    exit(1);
  }
  disk = fopen(argv[1], "rb");
  if (disk == NULL) {
    error("cannot open disk image file '%s'", argv[1]);
  }
  if (strcmp(argv[2], "*") == 0) {
    /* whole disk contains one single file system */
    fsStart = 0;
    fseek(disk, 0, SEEK_END);
    fsSize = ftell(disk) / SECTOR_SIZE;
  } else {
    /* argv[2] is partition number of file system */
    part = strtoul(argv[2], &endptr, 10);
    if (*endptr != '\0' || part < 0 || part > 15) {
      error("illegal partition number '%s'", argv[2]);
    }
    fseek(disk, 1 * SECTOR_SIZE, SEEK_SET);
    if (fread(partTable, 1, SECTOR_SIZE, disk) != SECTOR_SIZE) {
      error("cannot read partition table of disk '%s'", argv[1]);
    }
    ptptr = partTable + part * 32;
    partType = get4Bytes(ptptr + 0);
    if ((partType & 0x7FFFFFFF) != 0x00000058) {
      error("partition %d of disk '%s' does not contain an EOS32 file system",
            part, argv[1]);
    }
    fsStart = get4Bytes(ptptr + 4);
    fsSize = get4Bytes(ptptr + 8);
  }
  printf("File system has size %lu (0x%lX) sectors of %d bytes each.\n",
         fsSize, fsSize, SECTOR_SIZE);
  if (fsSize % SPB != 0) {
    printf("File system size is not a multiple of block size.\n");
  }
  numBlocks = fsSize / SPB;
  printf("This equals %lu (0x%lX) blocks of %d bytes each.\n",
         numBlocks, numBlocks, BLOCK_SIZE);
  if (numBlocks < 2) {
    error("file system has less than 2 blocks");
  }
  currBlock = 1;
  readBlock(disk, currBlock, blockBuffer);
  help();
  quit = 0;
  while (!quit) {
    printf("shfs [block %lu (0x%lX)] > ", currBlock, currBlock);
    fflush(stdout);
    if (fgets(line, LINE_SIZE, stdin) == NULL) {
      printf("\n");
      break;
    }
    if (line[0] == '\0' || line[0] == '\n') {
      continue;
    }
    switch (line[0]) {
      case 'h':
      case '?':
        help();
        break;
      case 'q':
        quit = 1;
        break;
      case 'r':
        rawBlock(blockBuffer);
        break;
      case 's':
        superBlock(blockBuffer);
        break;
      case 'i':
        inodeBlock(blockBuffer);
        break;
      case 'd':
        directoryBlock(blockBuffer);
        break;
      case 'f':
        freeBlock(blockBuffer);
        break;
      case '*':
        indirectBlock(blockBuffer);
        break;
      case 'b':
        p = line + 1;
        if (!parseNumber(&p, &n)) {
          break;
        }
        if (*p != '\0' && *p != '\n') {
          printf("Error: cannot parse block number!\n");
          break;
        }
        if (n >= numBlocks) {
          printf("Error: block number too big for file system!\n");
          break;
        }
        currBlock = n;
        readBlock(disk, currBlock, blockBuffer);
        break;
      case '+':
        n = currBlock + 1;
        if (n >= numBlocks) {
          printf("Error: block number too big for file system!\n");
          break;
        }
        currBlock = n;
        readBlock(disk, currBlock, blockBuffer);
        break;
      case '-':
        n = currBlock - 1;
        if (n >= numBlocks) {
          printf("Error: block number too big for file system!\n");
          break;
        }
        currBlock = n;
        readBlock(disk, currBlock, blockBuffer);
        break;
      case 't':
        p = line + 1;
        if (!parseNumber(&p, &n)) {
          break;
        }
        if (*p != '\0' && *p != '\n') {
          printf("Error: cannot parse inode number!\n");
          break;
        }
        printf("inode %lu (0x%lX) is in block %lu (0x%lX), inode %lu\n",
               n, n, n / INOPB + 2, n / INOPB + 2, n % INOPB);
        break;
      default:
        printf("Unknown command, type 'h' for help!\n");
        break;
    }
  }
  fclose(disk);
  return 0;
}
