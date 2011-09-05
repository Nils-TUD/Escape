/*
 * gimmo2mmx.c -- convert GIMMIX MMO files to MMX files
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#define mm		0x98	/* the escape code of mmo format */
#define lop_quote	0x0	/* the quotation lopcode */
#define lop_loc		0x1	/* the location lopcode */
#define lop_skip	0x2	/* the skip lopcode */
#define lop_fixo	0x3	/* the octabyte-fix lopcode */
#define lop_fixr	0x4	/* the relative-fix lopcode */
#define lop_fixrx	0x5	/* extended relative-fix lopcode */
#define lop_file	0x6	/* the file name lopcode */
#define lop_line	0x7	/* the file position lopcode */
#define lop_spec	0x8	/* the special hook lopcode */
#define lop_pre		0x9	/* the preamble lopcode */
#define lop_post	0xa	/* the postamble lopcode */
#define lop_stab	0xb	/* the symbol table lopcode */
#define lop_end		0xc	/* the end-it-all lopcode */

#define ybyte		buf[2]	/* the next-to-least significant byte */
#define zbyte		buf[3]	/* the least significant byte */

#define mmoLoad(loc, val)	{ ll = memFind(loc); ll->tet ^= val; }


/**************************************************************/


typedef unsigned char byte;	/* a monobyte */
typedef unsigned int tetra;	/* a tetrabyte */
typedef struct {
  tetra h;
  tetra l;
} octa;				/* an octabyte */


typedef struct {
  tetra tet;		/* the tetrabyte of simulated memory */
  tetra freq;		/* the number of times it was used as instruction */
  unsigned char bkpt;	/* breakpoint information for this tetrabyte */
  unsigned char fileNo;		/* source file number, if known */
  unsigned short lineNo;	/* source line number, if known */
} MemTetra;


typedef struct mem_node_struct {
  octa loc;			/* location of the first of 512 tetrabytes */
  tetra stamp;				/* time stamp for treap balancing */
  struct mem_node_struct *left;		/* pointer to left subtree */
  struct mem_node_struct *right;	/* pointer to right subtree */
  MemTetra dat[512];		/* the chunk of simulated tetrabytes */
} MemNode;


typedef struct {
  char *name;		/* name of source file */
  int lineCount;	/* number of lines in the file */
  long *map;		/* pointer to map of file positions */
} FileNode;


/**************************************************************/


int debug = 0;		/* do we want debugging output? */

tetra priority;		/* pseudorandom time stamp counter */
MemNode *memRoot;	/* root of the treap */
MemNode *lastMem;	/* the memory node most recently read or written */

FILE *mmoFile;		/* the input file */
int postamble;		/* have we encountered lop_post? */
int byteCount;		/* index of the next-to-be-read byte */
byte buf[4];		/* the most recently read bytes */
int yzbytes;		/* the two least significant bytes */
int delta;		/* difference for relative fixup */
tetra tet;		/* buf bytes packed big-endianwise */
octa curLoc;		/* the current location */
int curFile;		/* the most recently selected file number */
int curLine;		/* the current position in curFile, if nonzero */
octa tmp;		/* an octabyte of temporary interest */
tetra objTime;		/* when the object file was created */
FileNode fileInfo[256];	/* data about each source file */
octa aux;		/* auxiliary data */

FILE *mmxFile;

int G, L;
octa g[256];		/* global registers */
octa instPtr;		/* instruction pointer */


/**************************************************************/


void error(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(2);
}


/**************************************************************/


octa incr(octa y, int delta) {
  octa x;

  x.h = y.h;
  x.l = y.l + delta;
  if (delta >= 0) {
    if (x.l < y.l) {
      x.h++;
    }
  } else {
    if (x.l > y.l) {
      x.h--;
    }
  }
  return x;
}


/**************************************************************/


MemNode *newMem(void) {
  MemNode *p;

  p = (MemNode *) calloc(1, sizeof(MemNode));
  if (p == NULL) {
    error("cannot allocate any more memory");
  }
  /* use Fibonacci hashing for stamp */
  p->stamp = priority;
  priority += 0x9e3779b9;  /* floor(2^32*(phi-1)) */
  return p;
}


MemTetra *memFind(octa addr) {
  octa key;
  int offset;
  MemNode *p;
  MemNode **q;
  MemNode **l, **r;

  p = lastMem;
  key.h = addr.h;
  key.l = addr.l & 0xfffff800;
  offset = addr.l & 0x7fc;
  if (p == NULL) {
    p = newMem();
    memRoot = p;
    memRoot->loc = key;
    lastMem = memRoot;
  }
  if (p->loc.l != key.l || p->loc.h != key.h) {
    //<Search for key in the treap, setting lastMem and p to its location 21>
    for (p = memRoot; p; ) {
      if (key.l == p->loc.l && key.h == p->loc.h) {
        /* tetrabyte found in this node */
        lastMem = p;
        return &p->dat[offset >> 2];
      }
      if ((key.l < p->loc.l && key.h <= p->loc.h) || key.h < p->loc.h) {
        p = p->left;
      } else {
        p = p->right;
      }
    }
    /* the tetrabyte is not yet in the treap, must insert a new node */
    for (p = memRoot, q = &memRoot;
         p != NULL && p->stamp < priority;
         p = *q) {
      if ((key.l < p->loc.l && key.h <= p->loc.h) || key.h < p->loc.h) {
        q = &p->left;
      } else {
        q = &p->right;
      }
    }
    *q = newMem();
    (*q)->loc = key;
    //<Fix up the subtrees of *q 22>
    l = &(*q)->left;
    r = &(*q)->right;
    while (p) {
      if ((key.l < p->loc.l && key.h <= p->loc.h) || key.h < p->loc.h) {
        *r = p;
        r = &p->left;
        p = *r;
      } else {
        *l = p;
        l = &p->right;
        p = *l;
      }
    }
    *l = NULL;
    *r = NULL;
    /* set p and lastMem */
    p = *q;
    lastMem = p;
  }
  return &p->dat[offset >> 2];
}


void initMemory(void) {
  priority = 314159265;
  memRoot = NULL;
  lastMem = memRoot;
}


/**************************************************************/


void dumpNybble(unsigned char nybble) {
  if (nybble < 10) {
    fputc('0' + nybble, mmxFile);
  } else {
    fputc('a' + nybble - 10, mmxFile);
  }
}


void dumpByte(unsigned char b) {
  dumpNybble((b >> 4) & 0xf);
  dumpNybble((b >> 0) & 0xf);
}


void dumpTetra(tetra t) {
  dumpByte((t >> 24) & 0xff);
  dumpByte((t >> 16) & 0xff);
  dumpByte((t >>  8) & 0xff);
  dumpByte((t >>  0) & 0xff);
}


void dumpOcta(tetra h, tetra l) {
  fputc(' ', mmxFile);
  dumpTetra(h);
  dumpTetra(l);
  fputc('\n', mmxFile);
}


void dumpAddress(tetra h, tetra l) {
  /* first 'translate' to physical address */
  h &= 0x0000FFFF;
  /* then dump */
  dumpByte((h >> 8) & 0xff);
  dumpByte((h >> 0) & 0xff);
  dumpTetra(l);
  fputc(':', mmxFile);
  fputc('\n', mmxFile);
}


void dump(MemNode *p) {
  int j;

  if (p->left) {
    dump(p->left);
  }
  if (debug) {
    printf("DEBUG: dumping 256 octabytes starting at 0x%08x%08x\n",
           p->loc.h, p->loc.l);
  }
  dumpAddress(p->loc.h, p->loc.l);
  for (j = 0; j < 512; j += 2) {
    dumpOcta(p->dat[j].tet, p->dat[j + 1].tet);
  }
  if (p->right) {
    dump(p->right);
  }
}


/**************************************************************/


void mmoError(void) {
  error("bad object file, try running mmotype");
}


void readTet(void) {
  if (fread(buf, 1, 4, mmoFile) != 4) {
    mmoError();
  }
  yzbytes = (buf[2] << 8) + buf[3];
  tet = (((buf[0] << 8) + buf[1]) << 16) + yzbytes;
  if (debug) {
    printf("DEBUG: next tetrabyte read = 0x%08x\n", tet);
  }
}


byte readByte(void) {
  byte b;

  if (byteCount == 0) {
    readTet();
  }
  b = buf[byteCount];
  byteCount = (byteCount + 1) & 3;
  return b;
}


void usage(char *myself) {
  fprintf(stderr, "Usage: %s <mmo file> <mmx file>\n", myself);
  exit(1);
}


int main(int argc, char *argv[]) {
  MemTetra *ll;
  int j;
  char *p;

  if (argc != 3) {
    usage(argv[0]);
  }
  mmoFile = fopen(argv[1], "rb");
  if (mmoFile == NULL) {
    error("cannot open MMO input file '%s'", argv[1]);
  }
  mmxFile = fopen(argv[2], "wt");
  if (mmxFile == NULL) {
    error("cannot open MMX output file '%s'", argv[2]);
  }
  initMemory();
  /* load MMO file */
  byteCount = 0;
  curLoc.h = 0;
  curLoc.l = 0;
  curFile = -1;
  curLine = 0;
  //load the preamble 28
  readTet();
  if (buf[0] != mm || buf[1] != lop_pre) {
    mmoError();
  }
  if (ybyte != 1) {
    mmoError();
  }
  if (zbyte == 0) {
    objTime = 0xffffffff;
  } else {
    j = zbyte - 1;
    readTet();
    objTime = tet;
    for (; j > 0; j--) {
      readTet();
    }
  }
  if (debug) {
    printf("DEBUG: preamble finished, file creation time = 0x%08x\n",
           objTime);
  }
  do {
    //load the next item 29
    readTet();
loop:
    if (debug) {
      printf("DEBUG: loop, current location = 0x%08x%08x\n",
             curLoc.h, curLoc.l);
    }
    if (buf[0] == mm) {
      switch (buf[1]) {
        case lop_quote:
          if (debug) {
            printf("DEBUG: lop_quote seen\n");
          }
          if (yzbytes != 1) {
            mmoError();
          }
          readTet();
          break;
        //cases for lopcodes in the main loop 33
        case lop_loc:
          if (debug) {
            printf("DEBUG: lop_loc seen\n");
          }
          if (zbyte == 2) {
            j = ybyte;
            readTet();
            curLoc.h = (j << 24) + tet;
          } else
          if (zbyte == 1) {
            curLoc.h = ybyte << 24;
          } else {
            mmoError();
          }
          readTet();
          curLoc.l = tet;
          continue;
        case lop_skip:
          if (debug) {
            printf("DEBUG: lop_skip seen\n");
          }
          curLoc = incr(curLoc, yzbytes);
          continue;
        case lop_fixo:
          if (debug) {
            printf("DEBUG: lop_fixo seen\n");
          }
          if (zbyte == 2) {
            j = ybyte;
            readTet();
            tmp.h = (j << 24) + tet;
          } else
          if (zbyte == 1) {
            tmp.h = ybyte << 24;
          } else {
            mmoError();
          }
          readTet();
          tmp.l = tet;
          if (debug) {
            printf("DEBUG: fixo %08x%08x: %08x%08x\n",
                   tmp.h, tmp.l, curLoc.h, curLoc.l);
          }
          mmoLoad(tmp, curLoc.h)
          mmoLoad(incr(tmp, 4), curLoc.l);
          continue;
        case lop_fixr:
          if (debug) {
            printf("DEBUG: lop_fixr seen\n");
          }
          delta = yzbytes;
          tmp = incr(curLoc,
                     -(delta >= 0x1000000 ? (delta & 0xffffff) - (1 << j) :
                                            delta) << 2);
          if (debug) {
            printf("DEBUG: fixr %08x%08x: %08x\n", tmp.h, tmp.l, delta);
          }
          mmoLoad(tmp, delta);
          continue;
        case lop_fixrx:
          if (debug) {
            printf("DEBUG: lop_fixrx seen\n");
          }
          j = yzbytes;
          if (j != 16 && j != 24) {
            mmoError();
          }
          readTet();
          delta = tet;
          if (delta & 0xfe000000) {
            mmoError();
          }
          tmp = incr(curLoc,
                     -(delta >= 0x1000000 ? (delta & 0xffffff) - (1 << j) :
                                            delta) << 2);
          if (debug) {
            printf("DEBUG: fixrx %08x%08x: %08x\n", tmp.h, tmp.l, delta);
          }
          mmoLoad(tmp, delta);
          continue;
        case lop_file:
          if (debug) {
            printf("DEBUG: lop_file seen\n");
          }
          if (fileInfo[ybyte].name) {
            if (zbyte) {
              mmoError();
            }
            curFile = ybyte;
          } else {
            if (!zbyte) {
              mmoError();
            }
            fileInfo[ybyte].name = (char *) calloc(4 * zbyte + 1, 1);
            if (!fileInfo[ybyte].name) {
              error("no room to store the file name");
            }
            curFile = ybyte;
            for (j = zbyte, p = fileInfo[ybyte].name; j > 0; j--, p += 4) {
              readTet();
              *(p + 0) = buf[0];
              *(p + 1) = buf[1];
              *(p + 2) = buf[2];
              *(p + 3) = buf[3];
            }
          }
          curLine = 0;
          continue;
        case lop_line:
          if (debug) {
            printf("DEBUG: lop_line seen\n");
          }
          if (curFile < 0) {
            mmoError();
          }
          curLine = yzbytes;
          continue;
        case lop_spec:
          if (debug) {
            printf("DEBUG: lop_spec seen\n");
          }
          while (1) {
            readTet();
            if (buf[0] == mm) {
              if (buf[1] != lop_quote || yzbytes != 1) {
                goto loop;
              }
              readTet();
            }
          }
        case lop_post:
          if (debug) {
            printf("DEBUG: lop_post seen\n");
          }
          postamble = 1;
          if (ybyte != 0 || zbyte < 32) {
            mmoError();
          }
          continue;
        default:
          mmoError();
      }
    }
    //load tet as a normal item 30
    if (debug) {
      printf("DEBUG: loading 0x%08x into 0x%08x%08x\n",
             tet, curLoc.h, curLoc.l);
    }
    mmoLoad(curLoc, tet);
    if (curLine != 0) {
      ll->fileNo = curFile;
      ll->lineNo = curLine;
      curLine++;
    }
    curLoc = incr(curLoc, 4);
    curLoc.l &= -4;
  } while (postamble == 0);
  //load the postamble 37
//  aux.h = 0x60000000;
//  aux.l = 0x18;
//  ll = memFind(aux);
//  (ll - 1)->tet = 2;  /* ultimately sets rL = 2 */
//  (ll - 5)->tet = argc;  /* and $0 = argc */
//  (ll - 4)->tet = 0x40000000;
//  (ll - 3)->tet = 0x8;  /* and $1 = Pool_Segment + 8 */
//  G = zbyte;
//  L = 0;
//  for (j = G + G; j < 256 + 256; j++, ll++, aux.l += 4) {
//    readTet();
//    ll->tet = tet;
//  }
//  instPtr.h = (ll - 2)->tet;  /* Main */
//  instPtr.l = (ll - 1)->tet;
//  (ll + 2 * 12)->tet = G << 24;
//  g[255] = incr(aux, 12 * 8);  /* we will UNSAVE from here */
  if (debug) {
    printf("DEBUG: postamble finished\n");
  }
  fclose(mmoFile);
  curLine = 0;
  /* store MMX file */
  dump(memRoot);
  fclose(mmxFile);
  /* done */
  return 0;
}
